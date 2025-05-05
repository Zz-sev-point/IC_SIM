#include <iostream>
#include <unordered_map>
#include <queue>
#include <memory>
#include <iomanip>
#include <fstream>
#include <algorithm>
#include <chrono>
#include "dgraph_logger.h"

constexpr uint32_t CROSSBAR_SIZE = 256;
constexpr uint32_t ACC_SIZE = CROSSBAR_SIZE;
constexpr uint32_t ACT_SIZE = CROSSBAR_SIZE;  
constexpr uint32_t BIT_PRECISION = 2;

struct Packet {
    uint32_t source;
    uint32_t destination;
    uint32_t size_bits;

    Packet(uint32_t src, uint32_t dest, uint32_t size)
        : source(src), destination(dest), size_bits(size) {}
};

struct Packets {
    uint32_t source;
    uint32_t destination;
    uint32_t size_bits;
    uint32_t times;

    Packets(uint32_t src, uint32_t dest, uint32_t size, uint32_t times)
        : source(src), destination(dest), size_bits(size), times(times) {}
};

class Interconnect;

// Generic Component class
class Component {
protected:
    uint32_t address;
    uint32_t size_bits; 
    Interconnect* interconnect;
    std::string type;

public:
    Component(uint32_t size, Interconnect* ic);

    void setAddr(uint32_t addr) { address = addr; }
        
    // Can be overided if needed
    virtual void receive(Packet packet) {
        std::cout << "[0x" << std::hex << address 
                  << "] Received packet from 0x" << packet.source 
                  << " | Packet Size: " << std::dec << packet.size_bits 
                  << " bits\n";
    }
    virtual void receive(Packets packets) {
        std::cout << "[0x" << std::hex << address 
                  << "] Received packets from 0x" << packets.source 
                  << " | Packet Size: " << std::dec << packets.times << "x " << std::dec << packets.size_bits 
                  << " bits\n";
    }

    virtual void send(uint32_t dest);
    void send(uint32_t dest, uint32_t size);
    void send(uint32_t dest, uint32_t size, uint32_t times);

    uint32_t getAddress() const { return address; }
    uint32_t getSize() const { return size_bits; }
    virtual std::string getType() { return "Not defined!"; }
};

// Interconnect model
class Interconnect {
private:
    std::unordered_map<uint32_t, Component*> address_map;
    uint32_t next_addr = 0x1000;
    DotGraphLogger logger;

public:
    Interconnect(const std::string& dotFileName)
        : logger(dotFileName) {}

    uint32_t registerComponent(Component* component) {
        uint32_t addr = next_addr;
        next_addr += 0x1000;
        component->setAddr(addr);
        address_map[addr] = component;
        return addr;
        // logger.addNode(component->getAddress(), component->getType());
    }

    void sendPacket(const Packet& packet) {
        if (address_map.find(packet.destination) != address_map.end()) {
            logger.addEdge(packet.source, address_map.find(packet.source)->second->getType(), packet.destination, address_map.find(packet.destination)->second->getType(), packet.size_bits, 1);
            address_map[packet.destination]->receive(packet);
        }
    }
    void sendPackets(const Packets& packets) {
        if (address_map.find(packets.destination) != address_map.end()) {
            logger.addEdge(packets.source, address_map.find(packets.source)->second->getType(), packets.destination, address_map.find(packets.destination)->second->getType(), packets.size_bits, packets.times);
            address_map[packets.destination]->receive(packets);
        }
    }
    uint32_t getNextAddr() { return next_addr; }
    std::string getType() { return "Interconnection"; }
};

Component::Component(uint32_t size, Interconnect* ic)
: size_bits(size), interconnect(ic) {
    if (!ic) {
        throw std::runtime_error("Interconnect pointer is null");
    }
}

// Component's send function
void Component::send(uint32_t dest) {
    Packet packet(address, dest, size_bits);
    interconnect->sendPacket(packet);
}
void Component::send(uint32_t dest, uint32_t size) {
    Packet packet(address, dest, size);
    interconnect->sendPacket(packet);
}
void Component::send(uint32_t dest, uint32_t size, uint32_t times) {
    Packets packets(address, dest, size, times);
    interconnect->sendPackets(packets);
}

class CIMCrossbar: public Component {
    private:
    uint32_t input_times = 0;
    uint32_t valid_rows = 0;
    uint32_t valid_volumes = 0;

    public:
    CIMCrossbar(uint32_t size, Interconnect* ic, uint32_t row_num, uint32_t vol_num) 
        : Component(size, ic) {
            valid_volumes = vol_num;
            valid_rows = row_num;
        }

    void processData(uint32_t dataSize) {
        std::cout << "[0x" << std::hex << address 
                  << "] Processing data | Data Size: " << std::dec << dataSize << " bits\n";
    }

    void receive(Packet packet) override {
        // if (size_bits < packet.size_bits) {
        //     std::cout << "Packet over size!" << std::endl;
        //     exit(1);
        // }
        std::cout << "[0x" << std::hex << address 
                  << "] Received data packet from 0x" << packet.source 
                  << " | Packet Size: " << std::dec << packet.size_bits 
                  << " bits. Processing...\n";
        processData(packet.size_bits); // Simulate processing the data
        input_times = 1;
    }

    void receive(Packets packets) override {
        if (size_bits < packets.size_bits) {
            std::cout << "Packets over size!" << std::endl;
            exit(1);
        }
        std::cout << "[0x" << std::hex << address 
                  << "] Received data packet from 0x" << packets.source 
                  << " | Packet Size: " << std::dec << packets.size_bits 
                  << " bits. Processing...\n";
        processData(packets.size_bits); // Simulate processing the data
        input_times = packets.times;
    }

    void send(uint32_t dest) override {
        Packets packets(address, dest, valid_volumes, input_times);
        interconnect->sendPackets(packets);
        input_times = 0;
    }

    std::string getType() { return "Crossbar"; }
};

class Host: public Component {
    public:
    Host(uint32_t size, Interconnect* ic) 
    : Component(size, ic) {}
    std::string getType() { return "Host"; }
};

class Accumulator: public Component {
    private:
    uint32_t input_times = 0;
    uint32_t compute_bits = 0;

    public:
    Accumulator(uint32_t size, Interconnect* ic)
        : Component(size, ic) {}

    void processData(uint32_t data_size, uint32_t data_times) {
        std::cout << "[0x" << std::hex << address 
                    << "] Accumulating data | Data Size: " << std::dec << data_times << "x " << std::dec << data_size << " bits\n";
    }

    void receive(Packets packets) override {
        if (size_bits < packets.size_bits) {
            std::cout << "Packets over size!" << std::endl;
            exit(1);
        }
        std::cout << "[0x" << std::hex << address 
                  << "] Received data packet from 0x" << packets.source 
                  << " | Packet Size: " << std::dec << packets.times << "x " << std::dec << packets.size_bits 
                  << " bits. Processing...\n";
        processData(packets.size_bits, packets.times); // Simulate processing the data
        input_times = packets.times;
        compute_bits = packets.size_bits;
    }
    void send(uint32_t dest) override {
        Packets packets(address, dest, compute_bits/BIT_PRECISION, input_times);
        interconnect->sendPackets(packets);
        input_times = 0;
        compute_bits = 0;
    }

    std::string getType() { return "Accumulator"; }

};

class Activation: public Component {
    private:
    std::string activation_type;
    uint32_t input_times = 0;
    uint32_t compute_bits = 0;
    
    public:
    Activation(uint32_t size, Interconnect* ic, std::string activation_type)
        : Component(size, ic), activation_type(activation_type) {}

    void processData(uint32_t dataSize) {
        std::cout << "[0x" << std::hex << address 
                    << "] Activating | Data Size: " << std::dec << dataSize << " bits\n";
    }

    void receive(Packets packets) override {
        if (size_bits < packets.size_bits) {
            std::cout << "Packet over size!" << std::endl;
            exit(1);
        }
        std::cout << "[0x" << std::hex << address 
                  << "] Received data packet from 0x" << packets.source 
                  << " | Packet Size: " << std::dec << packets.times << "x " << std::dec << packets.size_bits 
                  << " bits. Processing...\n";
        processData(packets.size_bits); // Simulate processing the data
        input_times = packets.times;
        compute_bits = packets.size_bits;
    }

    void send(uint32_t dest) override {
        Packets packets(address, dest, compute_bits, input_times);
        interconnect->sendPackets(packets);
    }
    std::string getType() { return "Activation"; }

};

uint32_t ceil_div(uint32_t a, uint32_t b) {
    return (a + b - 1) / b;
}

class Im2col: public Component {
    private:
    uint32_t kernel_size[3];
    uint32_t input_size[3];
    uint32_t stride;
    uint32_t pad;
    std::vector<uint32_t> packets_sizes;

    public:
    Im2col(uint32_t size, Interconnect* ic, uint32_t kernel_size[3], uint32_t input_size[3], uint32_t stride, uint32_t pad) // size is crossbar size
    : Component(size, ic), stride(stride), pad(pad) {
        std::copy(input_size, input_size + 3, this->input_size);
        std::copy(kernel_size, kernel_size + 3, this->kernel_size);
        uint32_t output_bits = kernel_size[0] * kernel_size[1] * input_size[2];
        while(1) {
            if (output_bits > size) {
                packets_sizes.emplace_back(size);
                output_bits -= size;
            } else {
                packets_sizes.emplace_back(output_bits);
                break;
            }
        }
    }

    void send(std::vector<uint32_t> addresses) {
        if (addresses.size() % packets_sizes.size() != 0) {
            std::cout << "Addresses error!" << std::endl;
            exit(1);
        }
        uint32_t packet_num = (input_size[0] - kernel_size[0] + 1 + pad * 2) / stride * (input_size[1] - kernel_size[1] + 1 + pad * 2) / stride;
        uint32_t count = 0;
        for(auto &addr: addresses) {
            Packets packets(address, addr, packets_sizes[count], packet_num);
            interconnect->sendPackets(packets);
            count = (count+1) % packets_sizes.size();
        }
    }
    std::string getType() { return "Im2col"; }
};

class Flatten: public Component {
    private:
    uint32_t total_bits = 0;
    public:
    Flatten(uint32_t size, Interconnect* ic): Component(size, ic) {}

    void receive(Packets packets) override {
        std::cout << "[0x" << std::hex << address 
                  << "] Received data packet from 0x" << packets.source 
                  << " | Packet Size: " << std::dec << packets.times << "x " << std::dec << packets.size_bits 
                  << " bits. Processing...\n";
        total_bits += packets.size_bits * packets.times;
    }

    void send(std::vector<uint32_t> addresses) {
        for(auto &addr: addresses) {
            if (size_bits < total_bits) {
                Packets packets(address, addr, size_bits, 1);
                interconnect->sendPackets(packets);
                total_bits -= size_bits;
            } else {
                Packets packets(address, addr, total_bits, 1);
                interconnect->sendPackets(packets);
            }
        }
    }

    std::string getType() { return "Flatten"; }
};

class Pool: public Component {
    private:
    std::string type;
    uint32_t input_bits = 0;
    std::vector<uint32_t> packets_sizes;

    public:
    Pool(uint32_t size, Interconnect* ic, std::string pooling_type)
        : Component(size, ic), type(pooling_type) {}

    void processData(uint32_t dataSize) {
        std::cout << "[0x" << std::hex << address 
                    << "] Pooling | Data Size: " << std::dec << dataSize << " bits\n";
        input_bits += dataSize;
    }

    void receive(Packet packet) override {
        std::cout << "[0x" << std::hex << address 
                  << "] Received data packet from 0x" << packet.source 
                  << " | Packet Size: " << std::dec << packet.size_bits 
                  << " bits. Processing...\n";
        processData(packet.size_bits); // Simulate processing the data
    }
    void receive(Packets packets) override {
        std::cout << "[0x" << std::hex << address 
                  << "] Received data packet from 0x" << packets.source 
                  << " | Packet Size: " << std::dec << packets.times << "x " << std::dec << packets.size_bits 
                  << " bits. Processing...\n";
        processData(packets.size_bits * packets.times); // Simulate processing the data
    }

    void pooling(uint32_t input_size[2], uint32_t kernel_size[1]) {
        if (input_bits < input_size[0] * input_size[1] * input_size[2]) {
            std::cout << "Input size error!" << std::endl;
            exit(1);
        }
        uint32_t old_size = input_size[0] * input_size[1];
        uint32_t new_size = (input_size[0] / kernel_size[0]) * (input_size[1] / kernel_size[1]);
        uint32_t output_bits = input_bits / old_size * new_size;
        // uint32_t output_bits = input_size[2] * new_size;
        input_bits = output_bits;
        while(1) {
            if (output_bits > size_bits) {
                packets_sizes.emplace_back(size_bits);
                output_bits -= size_bits;
            } else {
                packets_sizes.emplace_back(output_bits);
                break;
            }
        } 
    }

    void send(std::vector<uint32_t> addresses) {
        if (addresses.size() % packets_sizes.size() != 0) {
            std::cout << "Addresses error!" << std::endl;
            exit(1);
        }
        std::cout << "**" << addresses.size() << std::endl;
        std::cout << "**" << packets_sizes.size() << std::endl;
        std::cout << "**" << input_bits << std::endl;
        uint32_t count = 0;
        for(auto &addr: addresses) {
            Packet packet(address, addr, packets_sizes[count]);
            interconnect->sendPacket(packet);
            count = (count + 1) % packets_sizes.size();
        }
    }

    void send(uint32_t dest) {
        Packet packet(address, dest, input_bits);
        interconnect->sendPacket(packet);
    }

    std::string getType() { return type + " Pooling"; }
};

class NeuralNetworkLayer {
    protected:
    std::vector<CIMCrossbar> crossbars;
    std::vector<Accumulator> accumulators;
    std::vector<Activation> activations;
    uint32_t base_address, crossbar_vol_num, crossbar_row_num, crossbar_size;
    Interconnect* ic;

    void registerAll() {
        base_address = ic->getNextAddr();
        for (auto& c : crossbars) ic->registerComponent(&c);
        for (auto& a : accumulators) ic->registerComponent(&a);
        for (auto& a : activations) ic->registerComponent(&a);
    }

public:
    NeuralNetworkLayer(uint32_t crossbar_size, Interconnect* ic)
        : crossbar_size(crossbar_size), ic(ic) {}

    std::vector<uint32_t> get_input_addr() {
        std::vector<uint32_t> addresses;
        for (uint32_t i = 0; i < crossbar_row_num; i++) {
            for (uint32_t j = 0; j < crossbar_vol_num; j++) {
                addresses.emplace_back(base_address + (i * crossbar_vol_num + j) * 0x1000);
            }
        }
        return addresses;
    }

    void set_up(Component* component, uint32_t data_size) {
        uint32_t left_data = data_size;
        for (auto& addr: get_input_addr()) {
            if (left_data > crossbar_size) {
                component->send(addr, crossbar_size, BIT_PRECISION);
                left_data -= crossbar_size;
            } else {
                component->send(addr, left_data, BIT_PRECISION);
                left_data = data_size;
            }
        }
    }
};

class FullyConnectedLayer: public NeuralNetworkLayer {
    private:
    uint32_t input_size;
    uint32_t neural_num;
    public:
    FullyConnectedLayer(uint32_t input_size, uint32_t neural_num, uint32_t crossbar_size, Interconnect *ic, std::string type)
        : input_size(input_size), neural_num(neural_num), NeuralNetworkLayer(crossbar_size, ic) {
            uint32_t vol_num = neural_num;
            uint32_t row_num = input_size;
            uint32_t vol_num_p_crossbar = crossbar_size / BIT_PRECISION;
            crossbar_row_num = ceil_div(vol_num, vol_num_p_crossbar);
            crossbar_vol_num = ceil_div(row_num, crossbar_size);
            uint32_t total_num = crossbar_row_num * crossbar_vol_num; 

            uint32_t remain_vol_num = vol_num;
            for (uint32_t i = 0; i < crossbar_row_num; i++) {
                uint32_t remain_row_num = row_num;
                if (remain_vol_num > vol_num_p_crossbar) {
                    for (uint32_t j = 0; j < crossbar_vol_num; j++) {
                        if (remain_row_num > crossbar_size) {
                            crossbars.emplace_back(CIMCrossbar(crossbar_size, ic, crossbar_size, vol_num_p_crossbar * BIT_PRECISION));
                            remain_row_num -= crossbar_size;
                        } else {
                            crossbars.emplace_back(CIMCrossbar(crossbar_size, ic, remain_row_num, vol_num_p_crossbar * BIT_PRECISION));
                        }
                    }
                    remain_vol_num -= vol_num_p_crossbar;
                } else {
                    for (uint32_t j = 0; j < crossbar_vol_num; j++) {
                        if (remain_row_num > crossbar_size) {
                            crossbars.emplace_back(CIMCrossbar(crossbar_size, ic, crossbar_size, remain_vol_num * BIT_PRECISION));
                            remain_row_num -= crossbar_size;
                        } else {
                            crossbars.emplace_back(CIMCrossbar(crossbar_size, ic, remain_row_num, remain_vol_num * BIT_PRECISION));
                        }
                    }
                }
                accumulators.emplace_back(Accumulator(vol_num_p_crossbar * BIT_PRECISION, ic));
                activations.emplace_back(Activation(ACT_SIZE, ic, type));
            }
            registerAll();
        }
    
    void forward_propagation(std::vector<uint32_t> target_addresses) {
        uint32_t total_num = crossbar_row_num * crossbar_vol_num;

        for (uint32_t i = 0; i < crossbar_row_num; i++) {
            for (uint32_t j = 0; j < crossbar_vol_num; j++) {
                crossbars[i*crossbar_vol_num+j].send(base_address + (total_num + i) * 0x1000);
            }
            accumulators[i].send(base_address + (total_num + crossbar_row_num + i) * 0x1000);
        }

        uint32_t i = 0;
        uint32_t act_amount = activations.size();
        uint32_t addr_amount = target_addresses.size();
        if (act_amount <= addr_amount) {
            for (auto &addr: target_addresses) {
                activations[i%act_amount].send(addr);
                i++;
            }
        } else {
            for(auto &act: activations) {
                act.send(target_addresses[i%addr_amount]);
                i++;
            }
        }
    }

    void forward_propagation(uint32_t target_address) {
        uint32_t total_num = crossbar_row_num * crossbar_vol_num;

        for (uint32_t i = 0; i < crossbar_row_num; i++) {
            for (uint32_t j = 0; j < crossbar_vol_num; j++) {
                crossbars[i*crossbar_vol_num+j].send(base_address + (total_num + i) * 0x1000);
            }
            accumulators[i].send(base_address + (total_num + crossbar_row_num + i) * 0x1000);
            activations[i].send(target_address);
        }
    }
};

class ConvolutionLayer: public NeuralNetworkLayer {
    private:
    uint32_t input_size[3]; // 0-height, 1-width, 2-channel
    uint32_t kernel_size[3]; // 0-height, 1-width, 2-channel
    uint32_t stride;
    uint32_t pad;
    bool mapping_flag = true;
    Im2col _im2col;
    public:
    ConvolutionLayer(uint32_t input_size[3], uint32_t kernel_size[3], uint32_t stride, uint32_t pad, uint32_t crossbar_size, Interconnect *ic, std::string type)
        : NeuralNetworkLayer(crossbar_size, ic) , stride(stride), pad(pad), _im2col(crossbar_size, ic, kernel_size, input_size, stride, pad) {
            std::copy(input_size, input_size + 3, this->input_size);
            std::copy(kernel_size, kernel_size + 3, this->kernel_size);
            uint32_t img_row_num = input_size[0]-kernel_size[0]+1 + pad * 2;
            uint32_t img_vol_num = input_size[1]-kernel_size[1]+1 + pad * 2;
            if (img_row_num < 1 || img_vol_num < 1) {
                std::cout << "Illegal kernel size!" << std::endl;
                exit(1);
            }

            uint32_t row_num = 0;
            uint32_t vol_num = 0;
            if (mapping_flag) {
                row_num = input_size[0] * input_size[1] * input_size[2];
                vol_num = img_row_num / stride * img_vol_num / stride * kernel_size[2];
            } else {
                row_num = kernel_size[0] * kernel_size[1] * input_size[2];
                vol_num = kernel_size[2];
            }

            crossbar_row_num = ceil_div(vol_num, crossbar_size); 
            crossbar_vol_num = ceil_div(row_num, crossbar_size);
            uint32_t total_num = crossbar_row_num * crossbar_vol_num;

            uint32_t remain_vol_num = vol_num;
            for (uint32_t i = 0; i < crossbar_row_num; i++) {
                uint32_t remain_row_num = row_num;
                if (remain_vol_num > crossbar_size) {
                    for (uint32_t j = 0; j < crossbar_vol_num; j++) {
                        if (remain_row_num > crossbar_size) {
                            crossbars.emplace_back(CIMCrossbar(crossbar_size, ic, crossbar_size, crossbar_size));
                            remain_row_num -= crossbar_size;
                        } else {
                            crossbars.emplace_back(CIMCrossbar(crossbar_size, ic, remain_row_num, crossbar_size));
                        }
                    }
                    remain_vol_num -= crossbar_size;
                } else {
                    for (uint32_t j = 0; j < crossbar_vol_num; j++) {
                        if (remain_row_num > crossbar_size) {
                            crossbars.emplace_back(CIMCrossbar(crossbar_size, ic, crossbar_size, remain_vol_num));
                            remain_row_num -= crossbar_size;
                        } else {
                            crossbars.emplace_back(CIMCrossbar(crossbar_size, ic, remain_row_num, remain_vol_num));
                        }
                    }
                }
                accumulators.emplace_back(Accumulator(ACC_SIZE, ic));
                activations.emplace_back(Activation(ACT_SIZE, ic, type));
            }
            registerAll();
            if (!mapping_flag) { ic->registerComponent(&_im2col); }
        }

        std::vector<uint32_t> get_input_addr() {
            if (mapping_flag) {
                return NeuralNetworkLayer::get_input_addr();
            } else {
                return {base_address + crossbar_row_num * (crossbar_vol_num + 2) * 0x1000};
            }
        }

        void set_up(Component* component, uint32_t data_size) {
            if (mapping_flag) {
                NeuralNetworkLayer::set_up(component, data_size);
            } else {
                component->send(this->get_input_addr()[0], data_size);
            }
        }
        
        void forward_propagation(std::vector<uint32_t> target_addresses) {
            uint32_t total_num = crossbar_row_num * crossbar_vol_num;
            
            if (!mapping_flag) {
                std::vector<uint32_t> crossbar_addrs;
                for (uint32_t i = 0; i < crossbar_row_num; i++) {
                    for (uint32_t j = 0; j < crossbar_vol_num; j++) {
                        crossbar_addrs.emplace_back(crossbars[i*crossbar_vol_num+j].getAddress());
                    }
                }
                _im2col.send(crossbar_addrs);
            }
            for (uint32_t i = 0; i < crossbar_row_num; i++) {
                for (uint32_t j = 0; j < crossbar_vol_num; j++) {
                    crossbars[i*crossbar_vol_num+j].send(base_address + (total_num + i) * 0x1000);
                }
                accumulators[i].send(base_address + (total_num + crossbar_row_num + i) * 0x1000);
            }

            uint32_t i = 0;
            uint32_t act_amount = activations.size();
            uint32_t addr_amount = target_addresses.size();
            if (act_amount <= addr_amount) {
                for (auto &addr: target_addresses) {
                    activations[i%act_amount].send(addr);
                    i++;
                }
            } else {
                for(auto &act: activations) {
                    act.send(target_addresses[i%addr_amount]);
                    i++;
                }
            }
        }
    
        void forward_propagation(uint32_t target_address) {
            uint32_t total_num = crossbar_row_num * crossbar_vol_num;
    
            if (!mapping_flag) {
                std::vector<uint32_t> crossbar_addrs;
                for (uint32_t i = 0; i < crossbar_row_num; i++) {
                    for (uint32_t j = 0; j < crossbar_vol_num; j++) {
                        crossbar_addrs.emplace_back(crossbars[i*crossbar_vol_num+j].getAddress());
                    }
                }
                _im2col.send(crossbar_addrs);
            }
            for (uint32_t i = 0; i < crossbar_row_num; i++) {
                for (uint32_t j = 0; j < crossbar_vol_num; j++) {
                    crossbars[i*crossbar_vol_num+j].send(base_address + (total_num + i) * 0x1000);
                }
                accumulators[i].send(base_address + (total_num + crossbar_row_num + i) * 0x1000);
                activations[i].send(target_address);
            }
        }
};

class PoolingLayer: public NeuralNetworkLayer {
    private:
    uint32_t input_size[3]; // 0-height, 1-width, 2-channel
    uint32_t kernel_size[3]; // 0-height, 1-width, 2-channel
    Pool _pool;
    public:
    PoolingLayer(uint32_t input_size[3], uint32_t kernel_size[3], uint32_t crossbar_size, Interconnect *ic, std::string type)
        : NeuralNetworkLayer(crossbar_size, ic), _pool(crossbar_size, ic, type) {
            std::copy(input_size, input_size + 3, this->input_size);
            std::copy(kernel_size, kernel_size + 3, this->kernel_size);
            ic->registerComponent(&_pool);
        }

        uint32_t get_input_addr() {
            // std::vector<uint32_t> addresses;
            // for (uint32_t i = 0; i < ceil_div(input_size[0] * input_size[1] * input_size[2], crossbar_size); i++) { 
            //     addresses.emplace_back(_pool.getAddress());
            // }
            // return addresses;
            return _pool.getAddress();
        }
        
        void forward_propagation(std::vector<uint32_t> target_addresses) {
            _pool.pooling(input_size, kernel_size);
            _pool.send(target_addresses);
        }
    
        void forward_propagation(uint32_t target_address) {
            _pool.pooling(input_size, kernel_size);
            _pool.send(target_address);
        }
};

// Main Simulation
int main() {
    auto start = std::chrono::high_resolution_clock::now();

    std::string file_name = "network.dot";
    Interconnect interconnect(file_name);
    Host host = Host(64*1024*8, &interconnect);
    interconnect.registerComponent(&host);
    bool nn_flag = true;
    if (nn_flag) {
        FullyConnectedLayer hidden_layer_0 = FullyConnectedLayer(784, 512, CROSSBAR_SIZE, &interconnect, "relu"); // 784*512
        FullyConnectedLayer hidden_layer_1 = FullyConnectedLayer(512, 32, CROSSBAR_SIZE, &interconnect, "relu"); // 512*32
        FullyConnectedLayer output_layer = FullyConnectedLayer(32, 10, CROSSBAR_SIZE, &interconnect, "softmax"); // 32*10

        hidden_layer_0.set_up(&host, 28*28);
        hidden_layer_0.forward_propagation(hidden_layer_1.get_input_addr());
        hidden_layer_1.forward_propagation(output_layer.get_input_addr());
        output_layer.forward_propagation(host.getAddress());
    } else {
        uint32_t stride = 2;
        uint32_t pad = 0;
        uint32_t input_sz_0[3] = {28, 28, 1};
        uint32_t kernel_sz_0[3] = {3, 3, 32};
        ConvolutionLayer hidden_layer_0 = ConvolutionLayer(input_sz_0, kernel_sz_0, stride, pad, CROSSBAR_SIZE, &interconnect, "relu");
        uint32_t pool_input_sz_0[3] = {(26 + pad) / stride, (26 + pad) / stride, 32};
        uint32_t pool_kernel_sz_0[3] = {2, 2, 1};
        PoolingLayer pooling_layer_0 = PoolingLayer(pool_input_sz_0, pool_kernel_sz_0, CROSSBAR_SIZE, &interconnect, "Max");

        uint32_t input_sz_1[3] = {13, 13, 32};
        uint32_t kernel_sz_1[3] = {3, 3, 64};
        ConvolutionLayer hidden_layer_1 = ConvolutionLayer(input_sz_1, kernel_sz_1, stride, pad, CROSSBAR_SIZE, &interconnect, "relu");
        uint32_t pool_input_sz_1[3] = {(11 + pad) / stride, (11 + pad) / stride, 64};
        uint32_t pool_kernel_sz_1[3] = {2, 2, 1};
        PoolingLayer pooling_layer_1 = PoolingLayer(pool_input_sz_1, pool_kernel_sz_1, CROSSBAR_SIZE, &interconnect, "Max");

        uint32_t input_sz_2[3] = {5, 5, 64};
        uint32_t kernel_sz_2[3] = {3, 3, 64};
        ConvolutionLayer hidden_layer_2 = ConvolutionLayer(input_sz_2, kernel_sz_2, stride, pad, CROSSBAR_SIZE, &interconnect, "relu");
        // uint32_t pool_input_sz_2[3] = {3, 3, 64};
        // uint32_t pool_kernel_sz_2[3] = {2, 2, 1};
        // PoolingLayer pooling_layer_2 = PoolingLayer(pool_input_sz_2, pool_kernel_sz_2, CROSSBAR_SIZE, &interconnect, "Max");


        Flatten flatten(CROSSBAR_SIZE, &interconnect);
        interconnect.registerComponent(&flatten);
        FullyConnectedLayer hidden_layer_3 = FullyConnectedLayer(576, 64, CROSSBAR_SIZE, &interconnect, "relu"); // 576*64
        FullyConnectedLayer output_layer = FullyConnectedLayer(64, 10, CROSSBAR_SIZE, &interconnect, "relu"); // 64*10

        hidden_layer_0.set_up(&host, 28*28);
        hidden_layer_0.forward_propagation(pooling_layer_0.get_input_addr());
        pooling_layer_0.forward_propagation(hidden_layer_1.get_input_addr());
        hidden_layer_1.forward_propagation(pooling_layer_1.get_input_addr());
        pooling_layer_1.forward_propagation(hidden_layer_2.get_input_addr());
        hidden_layer_2.forward_propagation(flatten.getAddress());
        flatten.send(hidden_layer_3.get_input_addr());
        // pooling_layer_2.forward_propagation(hidden_layer_3.get_input_addr());
        hidden_layer_3.forward_propagation(output_layer.get_input_addr());
        output_layer.forward_propagation(host.getAddress());

    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    std::ofstream dotFile;
    dotFile.open("report.txt");
    dotFile << "Crossbar Size: " << CROSSBAR_SIZE << "*" << CROSSBAR_SIZE << "\n"
    << "Bit Precision: " << BIT_PRECISION << "\n"
    << "Sim Time Cost: " << duration.count() << "e-6 s\n";
    dotFile.close();
    return 0;
}
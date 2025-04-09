#include <iostream>
#include <unordered_map>
#include <queue>
#include <memory>
#include <iomanip>
#include <fstream>
#include <algorithm>
#include <chrono>
#include "dgraph_logger.h"

constexpr uint32_t CROSSBAR_SIZE = 1024;
constexpr uint32_t ACC_SIZE = CROSSBAR_SIZE;
constexpr uint32_t ACT_SIZE = CROSSBAR_SIZE;  
constexpr uint32_t NUM_ACCURACY = 1;

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

class CIMCrossbar: public Component {
    private:
    uint32_t input_bits = 0;
    uint32_t input_times = 0;

    public:
    CIMCrossbar(uint32_t size, Interconnect* ic) 
        : Component(size, ic) {}

    void processData(uint32_t dataSize) {
        std::cout << "[0x" << std::hex << address 
                  << "] Processing data | Data Size: " << std::dec << dataSize << " bits\n";
    }

    void receive(Packet packet) override {
        if (size_bits < packet.size_bits) {
            std::cout << "Packet over size!" << std::endl;
            exit(1);
        }
        std::cout << "[0x" << std::hex << address 
                  << "] Received data packet from 0x" << packet.source 
                  << " | Packet Size: " << std::dec << packet.size_bits 
                  << " bits. Processing...\n";
        processData(packet.size_bits); // Simulate processing the data
        input_bits = packet.size_bits;
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
        input_bits = packets.size_bits;
        input_times = packets.times;
    }

    void send(uint32_t dest) override {
        Packets packets(address, dest, size_bits, input_bits*input_times);
        interconnect->sendPackets(packets);
        input_bits = 0;
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

    }
    std::string getType() { return "Accumulator"; }

};

class Activation: public Component {
    private:
    std::string activation_type;
    
    public:
    Activation(uint32_t size, Interconnect* ic, std::string activation_type)
        : Component(size, ic), activation_type(activation_type) {}

    void processData(uint32_t dataSize) {
        std::cout << "[0x" << std::hex << address 
                    << "] Activating | Data Size: " << std::dec << dataSize << " bits\n";
    }

    void receive(Packet packet) override {
        if (size_bits < packet.size_bits) {
            std::cout << "Packet over size!" << std::endl;
            exit(1);
        }
        std::cout << "[0x" << std::hex << address 
                  << "] Received data packet from 0x" << packet.source 
                  << " | Packet Size: " << std::dec << packet.size_bits 
                  << " bits. Processing...\n";
        processData(packet.size_bits); // Simulate processing the data
    }
    std::string getType() { return "Activation"; }

};

uint32_t ceil_div(uint32_t a, uint32_t b) {
    return (a + b - 1) / b;
}

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

    void get_input(Component* component, uint32_t data_size) {
        uint32_t left_data = data_size;
        for (auto& addr: get_input_addr()) {
            if (left_data > crossbar_size) {
                component->send(addr, crossbar_size);
                left_data -= crossbar_size;
            } else {
                component->send(addr, left_data);
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
            crossbar_row_num = ceil_div(neural_num, crossbar_size);
            crossbar_vol_num = ceil_div(input_size, crossbar_size);
            uint32_t total_num = crossbar_row_num * crossbar_vol_num;

            for (uint32_t i = 0; i < crossbar_row_num; i++) {
                for (uint32_t j = 0; j < crossbar_vol_num; j++) {
                    crossbars.emplace_back(CIMCrossbar(crossbar_size, ic));
                }
                accumulators.emplace_back(Accumulator(ACC_SIZE, ic));
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
        for (auto &addr: target_addresses) {
            activations[i%act_amount].send(addr);
            i++;
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
    public:
    ConvolutionLayer(uint32_t input_size[3], uint32_t kernel_size[3], uint32_t crossbar_size, Interconnect *ic, std::string type)
        : NeuralNetworkLayer(crossbar_size, ic) {
            std::copy(input_size, input_size + 3, this->input_size);
            std::copy(kernel_size, kernel_size + 3, this->kernel_size);
            uint32_t row_num = input_size[0]-kernel_size[0]+1;
            uint32_t vol_num = input_size[1]-kernel_size[1]+1;
            if (row_num < 1 || vol_num < 1) {
                std::cout << "Illegal kernel size!" << std::endl;
                exit(1);
            }
            crossbar_row_num = ceil_div(row_num * vol_num * kernel_size[2], CROSSBAR_SIZE); 
            crossbar_vol_num = ceil_div(input_size[0] * input_size[1] * input_size[2], CROSSBAR_SIZE);
            uint32_t total_num = crossbar_row_num * crossbar_vol_num;

            for (uint32_t i = 0; i < crossbar_row_num; i++) {
                for (uint32_t j = 0; j < crossbar_vol_num; j++) {
                    crossbars.emplace_back(CIMCrossbar(crossbar_size, ic));
                }
                accumulators.emplace_back(Accumulator(ACC_SIZE, ic));
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
            for (auto &addr: target_addresses) {
                activations[i%act_amount].send(addr);
                i++;
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

class PoolingLayer: public NeuralNetworkLayer {
    private:
    uint32_t input_size[3]; // 0-height, 1-width, 2-channel
    uint32_t kernel_size[3]; // 0-height, 1-width, 2-channel
    public:
    PoolingLayer(uint32_t input_size[3], uint32_t kernel_size[3], uint32_t crossbar_size, Interconnect *ic, std::string type)
        : NeuralNetworkLayer(crossbar_size, ic) {
            std::copy(input_size, input_size + 3, this->input_size);
            std::copy(kernel_size, kernel_size + 3, this->kernel_size);
            uint32_t row_num = input_size[0] / kernel_size[0];
            uint32_t vol_num = input_size[1] / kernel_size[1];
            if (row_num < 1 || vol_num < 1) {
                std::cout << "Illegal kernel size!" << std::endl;
                exit(1);
            }
            crossbar_row_num = ceil_div(row_num * vol_num * kernel_size[2], CROSSBAR_SIZE); 
            crossbar_vol_num = ceil_div(input_size[0] * input_size[1] * input_size[2], CROSSBAR_SIZE);
            uint32_t total_num = crossbar_row_num * crossbar_vol_num;

            for (uint32_t i = 0; i < crossbar_row_num; i++) {
                for (uint32_t j = 0; j < crossbar_vol_num; j++) {
                    crossbars.emplace_back(CIMCrossbar(crossbar_size, ic));
                }
                accumulators.emplace_back(Accumulator(ACC_SIZE, ic));
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
            for (auto &addr: target_addresses) {
                activations[i%act_amount].send(addr);
                i++;
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

// Main Simulation
int main() {
    auto start = std::chrono::high_resolution_clock::now();

    std::string file_name = "network.dot";
    Interconnect interconnect(file_name);
    Host host = Host(64*1024*8, &interconnect);
    interconnect.registerComponent(&host);
    bool nn_flag = false;
    if (nn_flag) {
        FullyConnectedLayer hidden_layer_0 = FullyConnectedLayer(784*NUM_ACCURACY, 512*NUM_ACCURACY, CROSSBAR_SIZE, &interconnect, "relu"); // 784*512
        FullyConnectedLayer hidden_layer_1 = FullyConnectedLayer(512*NUM_ACCURACY, 32*NUM_ACCURACY, CROSSBAR_SIZE, &interconnect, "relu"); // 512*32
        FullyConnectedLayer output_layer = FullyConnectedLayer(32*NUM_ACCURACY, 10, CROSSBAR_SIZE, &interconnect, "softmax"); // 32*10

        hidden_layer_0.get_input(&host, 28*28*NUM_ACCURACY*NUM_ACCURACY);
        hidden_layer_0.forward_propagation(hidden_layer_1.get_input_addr());
        hidden_layer_1.forward_propagation(output_layer.get_input_addr());
        output_layer.forward_propagation(host.getAddress());
    } else {
        uint32_t input_sz_0[3] = {28, 28, 1};
        uint32_t kernel_sz_0[3] = {3, 3, 32};
        ConvolutionLayer hidden_layer_0 = ConvolutionLayer(input_sz_0, kernel_sz_0, CROSSBAR_SIZE, &interconnect, "relu");
        uint32_t pool_input_sz_0[3] = {26, 26, 32};
        uint32_t pool_kernel_sz_0[3] = {2, 2, 1};
        PoolingLayer pooling_layer_0 = PoolingLayer(pool_input_sz_0, pool_kernel_sz_0, CROSSBAR_SIZE, &interconnect, "max");

        uint32_t input_sz_1[3] = {13, 13, 32};
        uint32_t kernel_sz_1[3] = {3, 3, 64};
        ConvolutionLayer hidden_layer_1 = ConvolutionLayer(input_sz_1, kernel_sz_1, CROSSBAR_SIZE, &interconnect, "relu");
        uint32_t pool_input_sz_1[3] = {11, 11, 64};
        uint32_t pool_kernel_sz_1[3] = {2, 2, 1};
        PoolingLayer pooling_layer_1 = PoolingLayer(pool_input_sz_1, pool_kernel_sz_1, CROSSBAR_SIZE, &interconnect, "max");

        uint32_t input_sz_2[3] = {5, 5, 64};
        uint32_t kernel_sz_2[3] = {3, 3, 64};
        ConvolutionLayer hidden_layer_2 = ConvolutionLayer(input_sz_2, kernel_sz_2, CROSSBAR_SIZE, &interconnect, "relu");
        uint32_t pool_input_sz_2[3] = {3, 3, 64};
        uint32_t pool_kernel_sz_2[3] = {2, 2, 1};
        PoolingLayer pooling_layer_2 = PoolingLayer(pool_input_sz_2, pool_kernel_sz_2, CROSSBAR_SIZE, &interconnect, "max");


        FullyConnectedLayer hidden_layer_3 = FullyConnectedLayer(64*NUM_ACCURACY, 64*NUM_ACCURACY, CROSSBAR_SIZE, &interconnect, "relu"); // 64*64
        FullyConnectedLayer output_layer = FullyConnectedLayer(64*NUM_ACCURACY, 10*NUM_ACCURACY, CROSSBAR_SIZE, &interconnect, "relu"); // 64*10

        hidden_layer_0.get_input(&host, 28*28*NUM_ACCURACY*NUM_ACCURACY);
        hidden_layer_0.forward_propagation(pooling_layer_0.get_input_addr());
        pooling_layer_0.forward_propagation(hidden_layer_1.get_input_addr());
        hidden_layer_1.forward_propagation(pooling_layer_1.get_input_addr());
        pooling_layer_1.forward_propagation(hidden_layer_2.get_input_addr());
        hidden_layer_2.forward_propagation(pooling_layer_2.get_input_addr());
        pooling_layer_2.forward_propagation(hidden_layer_3.get_input_addr());
        hidden_layer_3.forward_propagation(output_layer.get_input_addr());
        output_layer.forward_propagation(host.getAddress());

    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    std::ofstream dotFile;
    dotFile.open("report.txt");
    dotFile << "Crossbar Size: " << CROSSBAR_SIZE << "*" << CROSSBAR_SIZE << "\n"
    << "Bit Precision: " << NUM_ACCURACY << "\n"
    << "Sim Time Cost: " << duration.count() << "e-6 s\n";
    dotFile.close();
    return 0;
}
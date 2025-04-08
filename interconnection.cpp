#include <iostream>
#include <unordered_map>
#include <queue>
#include <memory>
#include <iomanip>
#include <fstream>
#include "dgraph_logger.h"

constexpr uint32_t CROSSBAR_SIZE = 256;
constexpr uint32_t ACC_SIZE = 512;
constexpr uint32_t ACT_SIZE = 512;  
constexpr uint32_t NUM_ACCURACY = 1;

struct Packet {
    uint32_t source;
    uint32_t destination;
    uint32_t size_bits;

    Packet(uint32_t src, uint32_t dest, uint32_t size)
        : source(src), destination(dest), size_bits(size) {}
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
    Component(uint32_t addr, uint32_t size, Interconnect* ic);
        
    // Can be overided if needed
    virtual void receive(Packet packet) {
        std::cout << "[0x" << std::hex << address 
                  << "] Received packet from 0x" << packet.source 
                  << " | Packet Size: " << std::dec << packet.size_bits 
                  << " bits | Component Size: " << size_bits << " bits\n";
    }

    void send(uint32_t dest, uint32_t size);

    uint32_t getAddress() const { return address; }
    uint32_t getSize() const { return size_bits; }
    virtual std::string getType() { return "Not defined!"; }
};

// Interconnect model
class Interconnect {
private:
    std::unordered_map<uint32_t, Component*> address_map;
    DotGraphLogger logger;

public:
    Interconnect(const std::string& dotFileName)
        : logger(dotFileName) {}

    void registerComponent(Component* component) {
        address_map[component->getAddress()] = component;
        logger.addNode(component->getAddress(), component->getType());
    }

    void sendPacket(const Packet& packet) {
        if (address_map.find(packet.destination) != address_map.end()) {
            logger.addEdge(packet.source, address_map.find(packet.source)->second->getType(), packet.destination, address_map.find(packet.destination)->second->getType(), packet.size_bits);
            address_map[packet.destination]->receive(packet);
        }
    }
    std::string getType() { return "Interconnection"; }
};

Component::Component(uint32_t addr, uint32_t size, Interconnect* ic)
: address(addr), size_bits(size), interconnect(ic) {
    if (!ic) {
        throw std::runtime_error("Interconnect pointer is null");
    }
}

// Component's send function
void Component::send(uint32_t dest, uint32_t size) {
    Packet packet(address, dest, size);
    interconnect->sendPacket(packet);
}

class CIMCrossbar: public Component {
    public:
    CIMCrossbar(uint32_t addr, uint32_t size, Interconnect* ic) 
        : Component(addr, size, ic) {}

    void processData(uint32_t dataSize) {
        std::cout << "[0x" << std::hex << address 
                  << "] Processing data | Data Size: " << std::dec << dataSize << " bits\n";
    }

    void receive(Packet packet) override {
        std::cout << "[0x" << std::hex << address 
                  << "] Received data packet from 0x" << packet.source 
                  << " | Packet Size: " << std::dec << packet.size_bits 
                  << " bits. Processing...\n";
        processData(packet.size_bits); // Simulate processing the data
    }
    std::string getType() { return "Crossbar"; }
};

class Host: public Component {
    public:
    Host(uint32_t addr, uint32_t size, Interconnect* ic) 
    : Component(addr, size, ic) {}
    std::string getType() { return "Host"; }
};

class Accumulator: public Component {
    public:
    Accumulator(uint32_t addr, uint32_t size, Interconnect* ic)
        : Component(addr, size, ic) {}

    void processData(uint32_t dataSize) {
        std::cout << "[0x" << std::hex << address 
                    << "] Accumulating data | Data Size: " << std::dec << dataSize << " bits\n";
    }

    void receive(Packet packet) override {
        std::cout << "[0x" << std::hex << address 
                  << "] Received data packet from 0x" << packet.source 
                  << " | Packet Size: " << std::dec << packet.size_bits 
                  << " bits. Processing...\n";
        processData(packet.size_bits); // Simulate processing the data
    }
    std::string getType() { return "Accumulator"; }

};

class Activation: public Component {
    private:
    std::string activation_type;
    
    public:
    Activation(uint32_t addr, uint32_t size, Interconnect* ic, std::string activation_type)
        : Component(addr, size, ic), activation_type(activation_type) {}

    void processData(uint32_t dataSize) {
        std::cout << "[0x" << std::hex << address 
                    << "] Activating | Data Size: " << std::dec << dataSize << " bits\n";
    }

    void receive(Packet packet) override {
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

class FullyConnectedLayer {
    private:
    uint32_t input_size;
    uint32_t neural_num;
    uint32_t crossbar_size;
    uint32_t base_address;
    uint32_t crossbar_row_num;
    uint32_t crossbar_vol_num;
    Interconnect *ic;
    std::vector<CIMCrossbar> crossbars;
    std::vector<Accumulator> accumulators;
    std::vector<Activation> activations;
    public:
    FullyConnectedLayer(uint32_t input_size, uint32_t neural_num, uint32_t crossbar_size, uint32_t address, Interconnect *ic, std::string type)
        : input_size(input_size), neural_num(neural_num), crossbar_size(crossbar_size), base_address(address), ic(ic) {
            crossbar_row_num = ceil_div(neural_num, crossbar_size);
            crossbar_vol_num = ceil_div(input_size, crossbar_size);
            uint32_t total_num = crossbar_row_num * crossbar_vol_num;

            for (uint32_t i = 0; i < crossbar_row_num; i++) {
                for (uint32_t j = 0; j < crossbar_vol_num; j++) {
                    crossbars.emplace_back(CIMCrossbar(base_address + (i * crossbar_vol_num + j) * 0x1000, crossbar_size * crossbar_size, ic));
                }
                accumulators.emplace_back(Accumulator(base_address + (total_num + i * 2) * 0x1000, ACC_SIZE, ic));
                activations.emplace_back(Activation(base_address + (total_num + i * 2 + 1) * 0x1000, ACT_SIZE, ic, type));
            }
            for (auto& crossbar: crossbars) {
                ic->registerComponent(&crossbar);
            }
            for (auto& acc: accumulators) {
                ic->registerComponent(&acc);
            }
            for (auto& act: activations) {
                ic->registerComponent(&act);
            }
        }
    
    std::vector<uint32_t> get_input_addr() {
        uint32_t total_num = crossbar_row_num * crossbar_vol_num;

        std::vector<uint32_t> addr;

        for (uint32_t i = 0; i < crossbar_row_num; i++) {
            for (uint32_t j = 0; j < crossbar_vol_num; j++) {
                addr.emplace_back(base_address + (i * crossbar_vol_num + j) * 0x1000);
            }
        }
        return addr;
    }
    
    void forward_propagation(std::vector<uint32_t> target_addresses) {
        uint32_t total_num = crossbar_row_num * crossbar_vol_num;

        for (uint32_t i = 0; i < crossbar_row_num; i++) {
            for (uint32_t j = 0; j < crossbar_vol_num; j++) {
                crossbars[i*crossbar_vol_num+j].send(base_address + (total_num + i * 2) * 0x1000, crossbar_size);
            }
            accumulators[i].send(base_address + (total_num + i * 2 + 1) * 0x1000, crossbar_size);
        }

        uint32_t i = 0;
        uint32_t act_amount = activations.size();
        for (auto &addr: target_addresses) {
            activations[i%act_amount].send(addr, crossbar_size);
            i++;
        }
    }

    void forward_propagation(uint32_t target_address) {
        uint32_t total_num = crossbar_row_num * crossbar_vol_num;

        for (uint32_t i = 0; i < crossbar_row_num; i++) {
            for (uint32_t j = 0; j < crossbar_vol_num; j++) {
                crossbars[i*crossbar_vol_num+j].send(base_address + (total_num + i * 2) * 0x1000, crossbar_size);
            }
            accumulators[i].send(base_address + (total_num + i * 2 + 1) * 0x1000, crossbar_size);
            activations[i].send(target_address, crossbar_size);
        }
    }
};

class ConvolutionLayer {
    private:
    uint32_t input_size[3]; // 0-height, 1-width, 2-channel
    uint32_t kernel_size[3]; // 0-height, 1-width, 2-channel
    uint32_t crossbar_size;
    uint32_t base_address;
    uint32_t crossbar_row_num;
    uint32_t crossbar_vol_num;
    Interconnect *ic;
    std::vector<CIMCrossbar> crossbars;
    std::vector<Accumulator> accumulators;
    std::vector<Activation> activations;
    public:
    ConvolutionLayer(uint32_t input_size[3], uint32_t kernel_size[3], uint32_t crossbar_size, uint32_t address, Interconnect *ic, std::string type)
        : crossbar_size(crossbar_size), base_address(address), ic(ic) {
            for (uint32_t i = 0; i < 3; i++) {
                this->input_size[i] = input_size[i];
                this->kernel_size[i] = kernel_size[i];
            }
            uint32_t row_num = input_size[0]-kernel_size[0]+1;
            uint32_t vol_num = input_size[1]-kernel_size[1]+1;
            if (row_num < 1 || vol_num < 1) {
                std::cout << "Illegal kernel size!" << std::endl;
            }
            crossbar_row_num = ceil_div(row_num * vol_num * kernel_size[2], CROSSBAR_SIZE); 
            crossbar_vol_num = ceil_div(input_size[0] * input_size[1] * input_size[2], CROSSBAR_SIZE);
            uint32_t total_num = crossbar_row_num * crossbar_vol_num;

            for (uint32_t i = 0; i < crossbar_row_num; i++) {
                for (uint32_t j = 0; j < crossbar_vol_num; j++) {
                    crossbars.emplace_back(CIMCrossbar(base_address + (i * crossbar_vol_num + j) * 0x1000, crossbar_size * crossbar_size, ic));
                }
                accumulators.emplace_back(Accumulator(base_address + (total_num + i * 2) * 0x1000, ACC_SIZE, ic));
                activations.emplace_back(Activation(base_address + (total_num + i * 2 + 1) * 0x1000, ACT_SIZE, ic, type));
            }
            for (auto& crossbar: crossbars) {
                ic->registerComponent(&crossbar);
            }
            for (auto& acc: accumulators) {
                ic->registerComponent(&acc);
            }
            for (auto& act: activations) {
                ic->registerComponent(&act);
            }
        }
    
        std::vector<uint32_t> get_input_addr() {
            uint32_t total_num = crossbar_row_num * crossbar_vol_num;
    
            std::vector<uint32_t> addr;
    
            for (uint32_t i = 0; i < crossbar_row_num; i++) {
                for (uint32_t j = 0; j < crossbar_vol_num; j++) {
                    addr.emplace_back(base_address + (i * crossbar_vol_num + j) * 0x1000);
                }
            }
            return addr;
        }
        
        void forward_propagation(std::vector<uint32_t> target_addresses) {
            uint32_t total_num = crossbar_row_num * crossbar_vol_num;
    
            for (uint32_t i = 0; i < crossbar_row_num; i++) {
                for (uint32_t j = 0; j < crossbar_vol_num; j++) {
                    crossbars[i*crossbar_vol_num+j].send(base_address + (total_num + i * 2) * 0x1000, crossbar_size);
                }
                accumulators[i].send(base_address + (total_num + i * 2 + 1) * 0x1000, crossbar_size);
            }
    
            uint32_t i = 0;
            uint32_t act_amount = activations.size();
            for (auto &addr: target_addresses) {
                activations[i%act_amount].send(addr, crossbar_size);
                i++;
            }
        }
    
        void forward_propagation(uint32_t target_address) {
            uint32_t total_num = crossbar_row_num * crossbar_vol_num;
    
            for (uint32_t i = 0; i < crossbar_row_num; i++) {
                for (uint32_t j = 0; j < crossbar_vol_num; j++) {
                    crossbars[i*crossbar_vol_num+j].send(base_address + (total_num + i * 2) * 0x1000, crossbar_size);
                }
                accumulators[i].send(base_address + (total_num + i * 2 + 1) * 0x1000, crossbar_size);
                activations[i].send(target_address, crossbar_size);
            }
        }
};

class PoolingLayer {
    private:
    uint32_t input_size[3]; // 0-height, 1-width, 2-channel
    uint32_t kernel_size[3]; // 0-height, 1-width, 2-channel
    uint32_t crossbar_size;
    uint32_t base_address;
    uint32_t crossbar_row_num;
    uint32_t crossbar_vol_num;
    Interconnect *ic;
    std::vector<CIMCrossbar> crossbars;
    std::vector<Accumulator> accumulators;
    std::vector<Activation> activations;
    public:
    PoolingLayer(uint32_t input_size[3], uint32_t kernel_size[3], uint32_t crossbar_size, uint32_t address, Interconnect *ic, std::string type)
        : crossbar_size(crossbar_size), base_address(address), ic(ic) {
            for (uint32_t i = 0; i < 3; i++) {
                this->input_size[i] = input_size[i];
                this->kernel_size[i] = kernel_size[i];
            }
            uint32_t row_num = input_size[0] / kernel_size[0];
            uint32_t vol_num = input_size[1] / kernel_size[1];
            if (row_num < 1 || vol_num < 1) {
                std::cout << "Illegal kernel size!" << std::endl;
            }
            crossbar_row_num = ceil_div(row_num * vol_num * kernel_size[2], CROSSBAR_SIZE); 
            crossbar_vol_num = ceil_div(input_size[0] * input_size[1] * input_size[2], CROSSBAR_SIZE);
            uint32_t total_num = crossbar_row_num * crossbar_vol_num;

            for (uint32_t i = 0; i < crossbar_row_num; i++) {
                for (uint32_t j = 0; j < crossbar_vol_num; j++) {
                    crossbars.emplace_back(CIMCrossbar(base_address + (i * crossbar_vol_num + j) * 0x1000, crossbar_size * crossbar_size, ic));
                }
                accumulators.emplace_back(Accumulator(base_address + (total_num + i * 2) * 0x1000, ACC_SIZE, ic));
                activations.emplace_back(Activation(base_address + (total_num + i * 2 + 1) * 0x1000, ACT_SIZE, ic, type));
            }
            for (auto& crossbar: crossbars) {
                ic->registerComponent(&crossbar);
            }
            for (auto& acc: accumulators) {
                ic->registerComponent(&acc);
            }
            for (auto& act: activations) {
                ic->registerComponent(&act);
            }
        }
    
        std::vector<uint32_t> get_input_addr() {
            uint32_t total_num = crossbar_row_num * crossbar_vol_num;
    
            std::vector<uint32_t> addr;
    
            for (uint32_t i = 0; i < crossbar_row_num; i++) {
                for (uint32_t j = 0; j < crossbar_vol_num; j++) {
                    addr.emplace_back(base_address + (i * crossbar_vol_num + j) * 0x1000);
                }
            }
            return addr;
        }
        
        void forward_propagation(std::vector<uint32_t> target_addresses) {
            uint32_t total_num = crossbar_row_num * crossbar_vol_num;
    
            for (uint32_t i = 0; i < crossbar_row_num; i++) {
                for (uint32_t j = 0; j < crossbar_vol_num; j++) {
                    crossbars[i*crossbar_vol_num+j].send(base_address + (total_num + i * 2) * 0x1000, crossbar_size);
                }
                accumulators[i].send(base_address + (total_num + i * 2 + 1) * 0x1000, crossbar_size);
            }
    
            uint32_t i = 0;
            uint32_t act_amount = activations.size();
            for (auto &addr: target_addresses) {
                activations[i%act_amount].send(addr, crossbar_size);
                i++;
            }
        }
    
        void forward_propagation(uint32_t target_address) {
            uint32_t total_num = crossbar_row_num * crossbar_vol_num;
    
            for (uint32_t i = 0; i < crossbar_row_num; i++) {
                for (uint32_t j = 0; j < crossbar_vol_num; j++) {
                    crossbars[i*crossbar_vol_num+j].send(base_address + (total_num + i * 2) * 0x1000, crossbar_size);
                }
                accumulators[i].send(base_address + (total_num + i * 2 + 1) * 0x1000, crossbar_size);
                activations[i].send(target_address, crossbar_size);
            }
        }
};

// Main Simulation
int main() {
    std::string file_name = "network.dot";
    Interconnect interconnect(file_name);
    Host host = Host(0x0000, 64*1024, &interconnect);
    interconnect.registerComponent(&host);
    bool nn_flag = true;
    if (nn_flag) {
        FullyConnectedLayer hidden_layer_0 = FullyConnectedLayer(784*NUM_ACCURACY, 512*NUM_ACCURACY, CROSSBAR_SIZE, 0x10000, &interconnect, "relu"); // 784*512
        FullyConnectedLayer hidden_layer_1 = FullyConnectedLayer(512*NUM_ACCURACY, 32*NUM_ACCURACY, CROSSBAR_SIZE, 0x40000, &interconnect, "relu"); // 512*32
        FullyConnectedLayer output_layer = FullyConnectedLayer(256*NUM_ACCURACY, 10, CROSSBAR_SIZE, 0x50000, &interconnect, "softmax"); // 32*10
        uint32_t total_num = ceil_div(784*NUM_ACCURACY, CROSSBAR_SIZE) * ceil_div(512*NUM_ACCURACY, CROSSBAR_SIZE);
        for (uint32_t i = 0; i < total_num; i++) {
            if (i < total_num - 1 || 28*28*NUM_ACCURACY*NUM_ACCURACY % CROSSBAR_SIZE == 0) {
                host.send(0x10000 + i * 0x1000, CROSSBAR_SIZE);
            } else {
                host.send(0x10000 + i * 0x1000, 28*28*NUM_ACCURACY*NUM_ACCURACY % CROSSBAR_SIZE);
            }
        }
        hidden_layer_0.forward_propagation(hidden_layer_1.get_input_addr());
        hidden_layer_1.forward_propagation(output_layer.get_input_addr());
        output_layer.forward_propagation(host.getAddress());
    } else {
        uint32_t input_sz_0[3] = {28, 28, 1};
        uint32_t kernel_sz_0[3] = {3, 3, 32};
        ConvolutionLayer hidden_layer_0 = ConvolutionLayer(input_sz_0, kernel_sz_0, CROSSBAR_SIZE, 0X10000, &interconnect, "relu");
        uint32_t pool_input_sz_0[3] = {26, 26, 32};
        uint32_t pool_kernel_sz_0[3] = {2, 2, 1};
        PoolingLayer pooling_layer_0 = PoolingLayer(pool_input_sz_0, pool_kernel_sz_0, CROSSBAR_SIZE, 0X40000, &interconnect, "max");

        uint32_t input_sz_1[3] = {13, 13, 32};
        uint32_t kernel_sz_1[3] = {3, 3, 64};
        ConvolutionLayer hidden_layer_1 = ConvolutionLayer(input_sz_1, kernel_sz_1, CROSSBAR_SIZE, 0X60000, &interconnect, "relu");
        uint32_t pool_input_sz_1[3] = {11, 11, 64};
        uint32_t pool_kernel_sz_1[3] = {2, 2, 1};
        PoolingLayer pooling_layer_1 = PoolingLayer(pool_input_sz_1, pool_kernel_sz_1, CROSSBAR_SIZE, 0X90000, &interconnect, "max");

        uint32_t input_sz_2[3] = {5, 5, 64};
        uint32_t kernel_sz_2[3] = {3, 3, 64};
        ConvolutionLayer hidden_layer_2 = ConvolutionLayer(input_sz_2, kernel_sz_2, CROSSBAR_SIZE, 0Xb0000, &interconnect, "relu");
        uint32_t pool_input_sz_2[3] = {3, 3, 64};
        uint32_t pool_kernel_sz_2[3] = {2, 2, 1};
        PoolingLayer pooling_layer_2 = PoolingLayer(pool_input_sz_2, pool_kernel_sz_2, CROSSBAR_SIZE, 0Xe0000, &interconnect, "max");


        FullyConnectedLayer hidden_layer_3 = FullyConnectedLayer(64*NUM_ACCURACY, 64*NUM_ACCURACY, CROSSBAR_SIZE, 0x100000, &interconnect, "relu"); // 64*64
        FullyConnectedLayer output_layer = FullyConnectedLayer(64*NUM_ACCURACY, 10*NUM_ACCURACY, CROSSBAR_SIZE, 0x110000, &interconnect, "relu"); // 64*10

        hidden_layer_0.forward_propagation(pooling_layer_0.get_input_addr());
        pooling_layer_0.forward_propagation(hidden_layer_1.get_input_addr());
        hidden_layer_1.forward_propagation(pooling_layer_1.get_input_addr());
        pooling_layer_1.forward_propagation(hidden_layer_2.get_input_addr());
        hidden_layer_2.forward_propagation(pooling_layer_2.get_input_addr());
        pooling_layer_2.forward_propagation(hidden_layer_3.get_input_addr());
        hidden_layer_3.forward_propagation(output_layer.get_input_addr());
        output_layer.forward_propagation(host.getAddress());

    }
    return 0;
}
#include "components.hpp"

uint32_t ceil_div(uint32_t a, uint32_t b) {
    return (a + b - 1) / b;
}

Packet::Packet(uint32_t src, uint32_t dest, uint32_t size)
    : source(src), destination(dest), size_bits(size) {}

Packets::Packets(uint32_t src, uint32_t dest, uint32_t size, uint32_t times)
    : source(src), destination(dest), size_bits(size), times(times) {}

// Generic Component class
Component::Component(uint32_t size, Interconnect* ic)
: size_bits(size), interconnect(ic) {
    if (!ic) {
        throw std::runtime_error("Interconnect pointer is null");
    }
}

void Component::setAddr(uint32_t addr) { address = addr; }
    
// Can be overided if needed
void Component::receive(Packet packet) {
    std::cout << "[0x" << std::hex << address 
                << "] Received packet from 0x" << packet.source 
                << " | Packet Size: " << std::dec << packet.size_bits 
                << " bits\n";
}
void Component::receive(Packets packets) {
    std::cout << "[0x" << std::hex << address 
                << "] Received packets from 0x" << packets.source 
                << " | Packet Size: " << std::dec << packets.times << "x " << std::dec << packets.size_bits 
                << " bits\n";
}

uint32_t Interconnect::registerComponent(Component* component) {
    uint32_t addr = next_addr;
    next_addr += UNIT_ADDR;
    component->setAddr(addr);
    address_map[addr] = component;
    if (component->getType() == "Crossbar") {
        crossbar_num++;
        CIMCrossbar* crossbar = dynamic_cast<CIMCrossbar*>(component);
        crossbar_valid_area += crossbar->getValidArea(); 
    }
    return addr;
}

void Interconnect::setBandWidth(uint32_t src_addr, uint32_t dest_addr, uint32_t bw) {
    if (address_map.find(src_addr) != address_map.end() && address_map.find(dest_addr) != address_map.end()) {
        this->bandwidth_map[{src_addr, dest_addr}] = bw;
        address_map[src_addr]->addOutPorts(1);
        address_map[dest_addr]->addInPorts(1);
    } else {
        std::cout << "Cannot find the target component!" << std::endl;
        exit(1);
    }
}

uint32_t Interconnect::getBandWidth(uint32_t src_addr, uint32_t dest_addr) {
    if (this->bandwidth_map.find({src_addr, dest_addr}) != this->bandwidth_map.end()) {
        return this->bandwidth_map[{src_addr, dest_addr}];
    } else {
        return 0;
    }
}

uint32_t Component::send(uint32_t dest) {
    Packet packet(address, dest, size_bits);
    return interconnect->sendPacket(packet);
}
uint32_t Component::send(uint32_t dest, uint32_t size) {
    Packet packet(address, dest, size);
    return interconnect->sendPacket(packet);
}
uint32_t Component::send(uint32_t dest, uint32_t size, uint32_t times) {
    Packets packets(address, dest, size, times);
    return interconnect->sendPackets(packets);
}

uint32_t Component::getAddress() { return address; }
uint32_t Component::getSize() { return size_bits; }

uint32_t Component::getInPortBW() { 
    if (in_port_num) {
        return ceil_div(in_port_bw, in_port_num);
    } else {
        return in_port_bw; 
    }
}

uint32_t Component::getOutPortBW() { 
    if (out_port_num) {
        return ceil_div(out_port_bw, out_port_num);
    } else {
        return out_port_bw; 
    }
}

uint32_t Component::getInPortNum() { return in_port_num; }
uint32_t Component::getOutPortNum(){ return out_port_num; }

uint32_t Component::addInPorts(uint32_t port_num) {
    in_port_num += port_num;
    return in_port_num;
}

uint32_t Component::addOutPorts(uint32_t port_num) {
    out_port_num += port_num;
    return out_port_num;
}

std::string Component::getType() { return "Not defined!"; }

// Interconnect model
Interconnect::Interconnect(const std::string& dotFileName)
    : logger(dotFileName) {}

uint32_t Interconnect::registerComponent(Component* component);

uint32_t Interconnect::sendPacket(const Packet& packet) {
    if (address_map.find(packet.destination) != address_map.end()) {
        if (address_map.find(packet.destination)->second->getType() != "Im2col" && min_bandwidth < packet.size_bits) {
            min_bandwidth = packet.size_bits;
        }

        total_bits_transferrd += static_cast<uint64_t>(packet.size_bits);
        logger.addEdge(packet.source, address_map.find(packet.source)->second->getType(), packet.destination, address_map.find(packet.destination)->second->getType(), packet.size_bits, 1);
        address_map[packet.destination]->receive(packet);

        uint32_t component_bw = std::min(address_map[packet.source]->getOutPortBW(), address_map[packet.destination]->getInPortBW());
        if (bandwidth_map.find({packet.source, packet.destination}) != bandwidth_map.end()) {
            uint32_t bw = std::min(component_bw, bandwidth_map[{packet.source, packet.destination}]);
            return ceil_div(packet.size_bits, bw) * UNIT_TIME;
        } else {
            return ceil_div(packet.size_bits, component_bw) * UNIT_TIME;
        }
    } else {
        std::cout << "Cannot find the target component!" << std::endl;
        exit(1);
    }
}
uint32_t Interconnect::sendPackets(const Packets& packets) {
    if (address_map.find(packets.destination) != address_map.end()) {
        if (address_map.find(packets.destination)->second->getType() != "Im2col" && min_bandwidth < packets.size_bits) {
            min_bandwidth = packets.size_bits;
        }

        total_bits_transferrd += static_cast<uint64_t>(packets.size_bits) * packets.times;
        logger.addEdge(packets.source, address_map.find(packets.source)->second->getType(), packets.destination, address_map.find(packets.destination)->second->getType(), packets.size_bits, packets.times);
        address_map[packets.destination]->receive(packets);

        uint32_t component_bw = std::min(address_map[packets.source]->getOutPortBW(), address_map[packets.destination]->getInPortBW());
        if (bandwidth_map.find({packets.source, packets.destination}) != bandwidth_map.end()) {
            uint32_t bw = std::min(component_bw, bandwidth_map[{packets.source, packets.destination}]);
            return ceil_div(packets.size_bits, bw) * packets.times * UNIT_TIME;
        } else {
            return ceil_div(packets.size_bits, component_bw) * packets.times * UNIT_TIME;
        }
    } else {
        std::cout << "Cannot find the target component!" << std::endl;
        exit(1);
    }
}
uint32_t Interconnect::getNextAddr() { return next_addr; }
std::string Interconnect::getType() { return "Interconnection"; }
uint32_t Interconnect::getCrossbarNum() { return crossbar_num; }
double Interconnect::getCrossbarUsage() { return static_cast<double>(crossbar_valid_area) / (crossbar_num * CROSSBAR_SIZE * CROSSBAR_SIZE); }

uint32_t Interconnect::getMinBandwidth() { return min_bandwidth; }
uint64_t Interconnect::getTotalBits() { return total_bits_transferrd; }

// CIMCrossbar
CIMCrossbar::CIMCrossbar(uint32_t size, Interconnect* ic, uint32_t row_num, uint32_t vol_num) 
    : Component(size, ic) {
        valid_volumes = vol_num;
        valid_rows = row_num;
        in_port_bw = CB_IN_BW;
        out_port_bw = CB_OUT_BW;
    }

void CIMCrossbar::processData(uint32_t dataSize) {
    std::cout << "[0x" << std::hex << address 
                << "] Processing data | Data Size: " << std::dec << dataSize << " bits\n";
}

void CIMCrossbar::receive(Packet packet) {
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

void CIMCrossbar::receive(Packets packets) {
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

uint32_t CIMCrossbar::send(uint32_t dest) {
    Packets packets(address, dest, valid_volumes, input_times);
    uint32_t delay = interconnect->sendPackets(packets);
    input_times = 0;
    return delay;
}

std::string CIMCrossbar::getType() { return "Crossbar"; }

uint32_t CIMCrossbar::getValidArea() {
    return valid_rows * valid_volumes;
}

uint32_t CIMCrossbar::getTimes() {
    return input_times;
}

Host::Host(uint32_t size, Interconnect* ic) 
: Component(size, ic) {}
std::string Host::getType() { return "Host"; }

// Accumulator
Accumulator::Accumulator(uint32_t size, Interconnect* ic)
    : Component(size, ic) {
        in_port_bw = ACC_IN_BW;
        out_port_bw = ACC_OUT_BW;
    }

void Accumulator::processData(uint32_t data_size, uint32_t data_times) {
    std::cout << "[0x" << std::hex << address 
                << "] Accumulating data | Data Size: " << std::dec << data_times << "x " << std::dec << data_size << " bits\n";
}

void Accumulator::receive(Packets packets) {
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

uint32_t Accumulator::send(uint32_t dest) {
    Packets packets(address, dest, compute_bits/BIT_PRECISION, input_times);
    uint32_t delay = interconnect->sendPackets(packets);
    input_times = 0;
    compute_bits = 0;
    return delay;
}

std::string Accumulator::getType() { return "Accumulator"; }

uint32_t Accumulator::getTimes() { return input_times; }

// Activation
Activation::Activation(uint32_t size, Interconnect* ic, std::string activation_type)
    : Component(size, ic), activation_type(activation_type) {
        in_port_bw = ACT_IN_BW;
        out_port_bw = ACT_OUT_BW;
    }

void Activation::processData(uint32_t dataSize) {
    std::cout << "[0x" << std::hex << address 
                << "] Activating | Data Size: " << std::dec << dataSize << " bits\n";
}

void Activation::receive(Packets packets) {
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

uint32_t Activation::send(uint32_t dest) {
    Packets packets(address, dest, compute_bits, input_times);
    return interconnect->sendPackets(packets);
}
std::string Activation::getType() { return "Activation"; }

uint32_t Activation::getTimes() { return input_times; }

// Im2col
Im2col::Im2col(uint32_t size, Interconnect* ic, uint32_t kernel_size[3], uint32_t input_size[3], uint32_t stride, uint32_t pad) // size is crossbar size
: Component(size, ic), stride(stride), pad(pad) {
    in_port_bw = IM_IN_BW;
    out_port_bw = IM_OUT_BW;
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

uint32_t Im2col::send(std::vector<uint32_t> addresses) {
    if (addresses.size() % packets_sizes.size() != 0) {
        std::cout << "Addresses error!" << std::endl;
        exit(1);
    }
    uint32_t packet_num = (input_size[0] - kernel_size[0] + 1 + pad * 2) / stride * (input_size[1] - kernel_size[1] + 1 + pad * 2) / stride * BIT_PRECISION;
    uint32_t count = 0;
    uint32_t delay = 0;
    for(auto &addr: addresses) {
        Packets packets(address, addr, packets_sizes[count], packet_num);
        delay = interconnect->sendPackets(packets);
        count = (count+1) % packets_sizes.size();
    }
    return delay;
}
std::string Im2col::getType() { return "Im2col"; }

// Flatten
Flatten::Flatten(uint32_t size, Interconnect* ic): Component(size, ic) {
    in_port_bw = FLATTEN_IN_BW;
    out_port_bw = FLATTEN_OUT_BW;
}

void Flatten::receive(Packets packets) {
    std::cout << "[0x" << std::hex << address 
                << "] Received data packet from 0x" << packets.source 
                << " | Packet Size: " << std::dec << packets.times << "x " << std::dec << packets.size_bits 
                << " bits. Processing...\n";
    total_bits += packets.size_bits * packets.times;
}

uint32_t Flatten::send(std::vector<uint32_t> addresses) {
    uint32_t delay = 0;
    for(auto &addr: addresses) {
        if (size_bits * BIT_PRECISION < total_bits) {
            Packets packets(address, addr, size_bits, BIT_PRECISION);
            delay = interconnect->sendPackets(packets);
            total_bits -= size_bits * BIT_PRECISION;
        } else {
            Packets packets(address, addr, total_bits / BIT_PRECISION, BIT_PRECISION);
            delay = interconnect->sendPackets(packets);
        }
    }
    return delay;
}

std::string Flatten::getType() { return "Flatten"; }

// Pool
Pool::Pool(uint32_t size, Interconnect* ic, std::string pooling_type)
    : Component(size, ic), type(pooling_type) {
        in_port_bw = POOL_IN_BW;
        out_port_bw = POOL_OUT_BW;  
    }

void Pool::processData(uint32_t dataSize) {
    std::cout << "[0x" << std::hex << address 
                << "] Pooling | Data Size: " << std::dec << dataSize << " bits\n";
    input_bits += dataSize;
}

void Pool::receive(Packet packet) {
    std::cout << "[0x" << std::hex << address 
                << "] Received data packet from 0x" << packet.source 
                << " | Packet Size: " << std::dec << packet.size_bits 
                << " bits. Processing...\n";
    processData(packet.size_bits); // Simulate processing the data
}
void Pool::receive(Packets packets) {
    std::cout << "[0x" << std::hex << address 
                << "] Received data packet from 0x" << packets.source 
                << " | Packet Size: " << std::dec << packets.times << "x " << std::dec << packets.size_bits 
                << " bits. Processing...\n";
    processData(packets.size_bits * packets.times); // Simulate processing the data
}

void Pool::pooling(uint32_t input_size[2], uint32_t kernel_size[1]) {
    in_port_bw = POOL_IN_BW;
    out_port_bw = POOL_OUT_BW;  
    uint32_t input_nums = input_bits / BIT_PRECISION;
    if (input_nums < input_size[0] * input_size[1] * input_size[2]) {
        std::cout << "Input size error!" << std::endl;
        exit(1);
    }
    uint32_t old_size = input_size[0] * input_size[1];
    uint32_t new_size = (input_size[0] / kernel_size[0]) * (input_size[1] / kernel_size[1]);
    uint32_t output_nums = input_nums / old_size * new_size;
    // uint32_t output_bits = input_size[2] * new_size;
    input_nums = output_nums;
    input_bits = input_nums * BIT_PRECISION;
    while(1) {
        if (output_nums > size_bits) {
            packets_sizes.emplace_back(size_bits);
            output_nums -= size_bits;
        } else {
            packets_sizes.emplace_back(output_nums);
            break;
        }
    } 
}

uint32_t Pool::send(std::vector<uint32_t> addresses) {
    if (addresses.size() % packets_sizes.size() != 0) {
        std::cout << "Addresses error!" << std::endl;
        exit(1);
    }
    uint32_t count = 0;
    uint32_t delay = 0;
    for(auto &addr: addresses) {
        Packets packets(address, addr, packets_sizes[count], BIT_PRECISION);
        delay = interconnect->sendPackets(packets);
        count = (count + 1) % packets_sizes.size();
    }
    return delay;
}

uint32_t Pool::send(uint32_t dest) {
    Packets packets(address, dest, input_bits/BIT_PRECISION, BIT_PRECISION);
    return interconnect->sendPackets(packets);
}

std::string Pool::getType() { return type + " Pooling"; }


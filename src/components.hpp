#pragma once
#include <iostream>
#include <unordered_map>
#include <algorithm>
#include "dgraph_logger.hpp"
#include "configuration.hpp"

uint32_t ceil_div(uint32_t a, uint32_t b);

struct Packet {
    uint32_t source;
    uint32_t destination;
    uint32_t size_bits;

    Packet(uint32_t src, uint32_t dest, uint32_t size);
};

struct Packets {
    uint32_t source;
    uint32_t destination;
    uint32_t size_bits;
    uint32_t times;

    Packets(uint32_t src, uint32_t dest, uint32_t size, uint32_t times);
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

    void setAddr(uint32_t addr);
        
    // Can be overided if needed
    virtual void receive(Packet packet);
    virtual void receive(Packets packets);

    virtual uint32_t send(uint32_t dest);
    virtual uint32_t send(uint32_t dest, uint32_t size);
    virtual uint32_t send(uint32_t dest, uint32_t size, uint32_t times);

    uint32_t getAddress();
    uint32_t getSize();
    virtual std::string getType();
    virtual ~Component() {}
};

// Interconnect model
class Interconnect {
private:
    std::unordered_map<uint32_t, Component*> address_map;
    uint32_t next_addr = UNIT_ADDR;
    DotGraphLogger logger;
    uint32_t crossbar_num = 0;
    uint32_t crossbar_valid_area = 0;
    uint32_t min_bandwidth = 0;
    uint64_t total_bits_transferrd = 0;

public:
    Interconnect(const std::string& dotFileName);

    uint32_t registerComponent(Component* component);

    void sendPacket(const Packet& packet);
    void sendPackets(const Packets& packets);
    uint32_t getNextAddr();
    std::string getType();
    uint32_t getCrossbarNum();
    double getCrossbarUsage();
    uint32_t getMinBandwidth();
    uint64_t getTotalBits();
};

class CIMCrossbar: public Component {
    private:
    uint32_t input_times = 0;
    uint32_t valid_rows = 0;
    uint32_t valid_volumes = 0;

    public:
    CIMCrossbar(uint32_t size, Interconnect* ic, uint32_t row_num, uint32_t vol_num);

    void processData(uint32_t dataSize);

    void receive(Packet packet) override;

    void receive(Packets packets) override;

    uint32_t send(uint32_t dest) override;

    std::string getType();

    uint32_t getValidArea();

    uint32_t getTimes();
};

class Host: public Component {
    public:
    Host(uint32_t size, Interconnect* ic);
    std::string getType();
};

class Accumulator: public Component {
    private:
    uint32_t input_times = 0;
    uint32_t compute_bits = 0;

    public:
    Accumulator(uint32_t size, Interconnect* ic);

    void processData(uint32_t data_size, uint32_t data_times);

    void receive(Packets packets) override;
    uint32_t send(uint32_t dest) override;

    std::string getType();

    uint32_t getTimes();
};

class Activation: public Component {
    private:
    std::string activation_type;
    uint32_t input_times = 0;
    uint32_t compute_bits = 0;
    
    public:
    Activation(uint32_t size, Interconnect* ic, std::string activation_type);

    void processData(uint32_t dataSize);

    void receive(Packets packets) override;
    uint32_t send(uint32_t dest) override;
    std::string getType();
    uint32_t getTimes();

};

class Im2col: public Component {
    private:
    uint32_t kernel_size[3];
    uint32_t input_size[3];
    uint32_t stride;
    uint32_t pad;
    std::vector<uint32_t> packets_sizes;

    public:
    Im2col(uint32_t size, Interconnect* ic, uint32_t kernel_size[3], uint32_t input_size[3], uint32_t stride, uint32_t pad); // size is crossbar size

    uint32_t send(std::vector<uint32_t> addresses);
    std::string getType();
};

class Flatten: public Component {
    private:
    uint32_t total_bits = 0;
    public:
    Flatten(uint32_t size, Interconnect* ic);

    void receive(Packets packets) override;

    uint32_t send(std::vector<uint32_t> addresses);

    std::string getType();
};

class Pool: public Component {
    private:
    std::string type;
    uint32_t input_bits = 0;
    std::vector<uint32_t> packets_sizes;

    public:
    Pool(uint32_t size, Interconnect* ic, std::string pooling_type);

    void processData(uint32_t dataSize);

    void receive(Packet packet) override;
    void receive(Packets packets) override;

    void pooling(uint32_t input_size[2], uint32_t kernel_size[1]);

    uint32_t send(std::vector<uint32_t> addresses);

    uint32_t send(uint32_t dest);

    std::string getType();
};


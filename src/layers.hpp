#pragma once
#include <chrono>
#include "components.hpp"

class NeuralNetworkLayer {
    protected:
    std::vector<CIMCrossbar> crossbars;
    std::vector<Accumulator> accumulators;
    std::vector<Activation> activations;
    uint32_t base_address, crossbar_vol_num, crossbar_row_num, crossbar_size;
    Interconnect* ic;

    uint32_t times = 0;

    void registerAll();

public:
    NeuralNetworkLayer(uint32_t crossbar_size, Interconnect* ic);

    virtual std::vector<uint32_t> get_input_addr();

    virtual uint32_t set_up(Component* component, uint32_t data_size);

    virtual void forward_propagation(std::vector<uint32_t> target_addresses);

    virtual void forward_propagation(uint32_t target_address);

    uint32_t get_delay();
};

class FullyConnectedLayer: public NeuralNetworkLayer {
    private:
    uint32_t input_size;
    uint32_t neural_num;
    public:
    FullyConnectedLayer(uint32_t input_size, uint32_t neural_num, uint32_t crossbar_size, Interconnect *ic, std::string type);
    
    void forward_propagation(std::vector<uint32_t> target_addresses) override;

    void forward_propagation(uint32_t target_address) override;
};

class ConvolutionLayer: public NeuralNetworkLayer {
    private:
    uint32_t input_size[3]; // 0-height, 1-width, 2-channel
    uint32_t kernel_size[3]; // 0-height, 1-width, 2-channel
    uint32_t stride;
    uint32_t pad;
    bool mapping_flag = false;
    Im2col _im2col;
    public:
    ConvolutionLayer(uint32_t input_size[3], uint32_t kernel_size[3], uint32_t stride, uint32_t pad, uint32_t crossbar_size, Interconnect *ic, std::string type);

    std::vector<uint32_t> get_input_addr() override;

    uint32_t set_up(Component* component, uint32_t data_size) override;
    
    void forward_propagation(std::vector<uint32_t> target_addresses) override;

    void forward_propagation(uint32_t target_address) override;
};

class PoolingLayer: public NeuralNetworkLayer {
    private:
    uint32_t input_size[3]; // 0-height, 1-width, 2-channel
    uint32_t kernel_size[3]; // 0-height, 1-width, 2-channel
    Pool _pool;
    public:
    PoolingLayer(uint32_t input_size[3], uint32_t kernel_size[3], uint32_t crossbar_size, Interconnect *ic, std::string type);

    std::vector<uint32_t> get_input_addr() override;
    
    void forward_propagation(std::vector<uint32_t> target_addresses) override;

    void forward_propagation(uint32_t target_address) override;
};

class FlattenLayer: public NeuralNetworkLayer {
    private:
    Flatten _flatten;
    public:
    FlattenLayer(uint32_t crossbar_size, Interconnect *ic);

    std::vector<uint32_t> get_input_addr() override;

    void forward_propagation(std::vector<uint32_t> target_addresses) override;
};
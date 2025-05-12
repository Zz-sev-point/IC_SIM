#include "layers.hpp"

void NeuralNetworkLayer::registerAll() {
    base_address = ic->getNextAddr();
    for (auto& c : crossbars) ic->registerComponent(&c);
    for (auto& a : accumulators) ic->registerComponent(&a);
    for (auto& a : activations) ic->registerComponent(&a);
}

NeuralNetworkLayer::NeuralNetworkLayer(uint32_t crossbar_size, Interconnect* ic)
        : crossbar_size(crossbar_size), ic(ic) {}

std::vector<uint32_t> NeuralNetworkLayer::get_input_addr() {
    std::vector<uint32_t> addresses;
    for (uint32_t i = 0; i < crossbar_row_num; i++) {
        for (uint32_t j = 0; j < crossbar_vol_num; j++) {
            addresses.emplace_back(base_address + (i * crossbar_vol_num + j) * 0x1000);
        }
    }
    return addresses;
}

uint32_t NeuralNetworkLayer::set_up(Component* component, uint32_t data_size) {
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
    return BIT_PRECISION * UNIT_TIME;
}

void NeuralNetworkLayer::forward_propagation(std::vector<uint32_t> target_addresses) {}

void NeuralNetworkLayer::forward_propagation(uint32_t target_address) {}

uint32_t NeuralNetworkLayer::get_delay() { return times; }

FullyConnectedLayer::FullyConnectedLayer(uint32_t input_size, uint32_t neural_num, uint32_t crossbar_size, Interconnect *ic, std::string type)
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

void FullyConnectedLayer::forward_propagation(std::vector<uint32_t> target_addresses){
    uint32_t total_num = crossbar_row_num * crossbar_vol_num;

    uint32_t crossbar_times = 0;
    uint32_t acc_times = 0;
    uint32_t act_times = 0;

    for (uint32_t i = 0; i < crossbar_row_num; i++) {
        for (uint32_t j = 0; j < crossbar_vol_num; j++) {
            uint32_t cb_t = crossbars[i*crossbar_vol_num+j].send(base_address + (total_num + i) * 0x1000);
            if (crossbar_times < cb_t) {
                crossbar_times = cb_t;
            }
        }
        uint32_t acc_t = accumulators[i].send(base_address + (total_num + crossbar_row_num + i) * 0x1000);
        if (acc_times < acc_t) {
            acc_times = acc_t;
        }
    }

    uint32_t i = 0;
    uint32_t act_amount = activations.size();
    uint32_t addr_amount = target_addresses.size();
    if (act_amount <= addr_amount) {
        for (auto &addr: target_addresses) {
            uint32_t act_t = activations[i%act_amount].send(addr);
            if (act_times < act_t) {
                act_times = act_t;
            }
            i++;
        }
    } else {
        for(auto &act: activations) {
            uint32_t act_t = act.send(target_addresses[i%addr_amount]);
            if (act_times < act_t) {
                act_times = act_t;
            }
            i++;
        }
    }
    this->times += (crossbar_times + acc_times + act_times);
}

void FullyConnectedLayer::forward_propagation(uint32_t target_address) {
    uint32_t total_num = crossbar_row_num * crossbar_vol_num;

    uint32_t crossbar_times = 0;
    uint32_t acc_times = 0;
    uint32_t act_times = 0;

    for (uint32_t i = 0; i < crossbar_row_num; i++) {
        for (uint32_t j = 0; j < crossbar_vol_num; j++) {
            uint32_t cb_t = crossbars[i*crossbar_vol_num+j].send(base_address + (total_num + i) * 0x1000);
            if (crossbar_times < cb_t) {
                crossbar_times = cb_t;
            }
        }
        uint32_t acc_t = accumulators[i].send(base_address + (total_num + crossbar_row_num + i) * 0x1000);
        uint32_t act_t = activations[i].send(target_address);
        if (acc_times < acc_t) {
            acc_times = acc_t;
        }
        if (act_times < act_t) {
            act_times = act_t;
        }
    }
    this->times += (crossbar_times + acc_times + act_times);
}

ConvolutionLayer::ConvolutionLayer(uint32_t input_size[3], uint32_t kernel_size[3], uint32_t stride, uint32_t pad, uint32_t crossbar_size, Interconnect *ic, std::string type)
: NeuralNetworkLayer(crossbar_size, ic) , stride(stride), pad(pad), _im2col(crossbar_size, ic, kernel_size, input_size, stride, pad) {
    std::copy(input_size, input_size + 3, this->input_size);
    std::copy(kernel_size, kernel_size + 3, this->kernel_size);
    uint32_t img_row_num = input_size[0]-kernel_size[0]+1 + pad * 2;
    uint32_t img_vol_num = input_size[1]-kernel_size[1]+1 + pad * 2;
    if (img_row_num < 1 || img_vol_num < 1) {
        std::cout << "Illegal kernel size!" << std::endl;
        exit(1);
    }

    uint32_t vol_num_p_crossbar = crossbar_size / BIT_PRECISION;
    uint32_t row_num = 0;
    uint32_t vol_num = 0;
    if (mapping_flag) {
        row_num = input_size[0] * input_size[1] * input_size[2];
        vol_num = img_row_num / stride * img_vol_num / stride * kernel_size[2];
    } else {
        row_num = kernel_size[0] * kernel_size[1] * input_size[2];
        vol_num = kernel_size[2];
    }

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
        accumulators.emplace_back(Accumulator(ACC_SIZE, ic));
        activations.emplace_back(Activation(ACT_SIZE, ic, type));
    }
    registerAll();
    if (!mapping_flag) { ic->registerComponent(&_im2col); }
}

std::vector<uint32_t> ConvolutionLayer::get_input_addr() {
    if (mapping_flag) {
        return NeuralNetworkLayer::get_input_addr();
    } else {
        return {base_address + crossbar_row_num * (crossbar_vol_num + 2) * 0x1000};
    }
}

uint32_t ConvolutionLayer::set_up(Component* component, uint32_t data_size) {
    if (mapping_flag) {
        return NeuralNetworkLayer::set_up(component, data_size);
    } else {
        return component->send(this->get_input_addr()[0], data_size) * UNIT_TIME;
    }
}

void ConvolutionLayer::forward_propagation(std::vector<uint32_t> target_addresses) {
    uint32_t total_num = crossbar_row_num * crossbar_vol_num;

    uint32_t crossbar_times = 0;
    uint32_t acc_times = 0;
    uint32_t act_times = 0;
    
    if (!mapping_flag) {
        std::vector<uint32_t> crossbar_addrs;
        for (uint32_t i = 0; i < crossbar_row_num; i++) {
            for (uint32_t j = 0; j < crossbar_vol_num; j++) {
                crossbar_addrs.emplace_back(crossbars[i*crossbar_vol_num+j].getAddress());
            }
        }
        this->times += _im2col.send(crossbar_addrs);
    }
    for (uint32_t i = 0; i < crossbar_row_num; i++) {
        for (uint32_t j = 0; j < crossbar_vol_num; j++) {
            uint32_t cb_t = crossbars[i*crossbar_vol_num+j].send(base_address + (total_num + i) * 0x1000);
            if (crossbar_times < cb_t) {
                crossbar_times = cb_t;
            }
        }
        uint32_t acc_t = accumulators[i].send(base_address + (total_num + crossbar_row_num + i) * 0x1000);
        if (acc_times < acc_t) {
            acc_times = acc_t;
        }
    }

    uint32_t i = 0;
    uint32_t act_amount = activations.size();
    uint32_t addr_amount = target_addresses.size();
    if (act_amount <= addr_amount) {
        for (auto &addr: target_addresses) {
            uint32_t act_t = activations[i%act_amount].send(addr);
            if (act_times < act_t) {
                act_times = act_t;
            }
            i++;
        }
    } else {
        for(auto &act: activations) {
            uint32_t act_t = act.send(target_addresses[i%addr_amount]);
            if (act_times < act_t) {
                act_times = act_t;
            }
            i++;
        }
    }
    this->times += (crossbar_times + acc_times + act_times);
}

void ConvolutionLayer::forward_propagation(uint32_t target_address) {
    uint32_t total_num = crossbar_row_num * crossbar_vol_num;

    uint32_t crossbar_times = 0;
    uint32_t acc_times = 0;
    uint32_t act_times = 0;

    if (!mapping_flag) {
        std::vector<uint32_t> crossbar_addrs;
        for (uint32_t i = 0; i < crossbar_row_num; i++) {
            for (uint32_t j = 0; j < crossbar_vol_num; j++) {
                crossbar_addrs.emplace_back(crossbars[i*crossbar_vol_num+j].getAddress());
            }
        }
        this->times += _im2col.send(crossbar_addrs);
    }
    for (uint32_t i = 0; i < crossbar_row_num; i++) {
        for (uint32_t j = 0; j < crossbar_vol_num; j++) {
            uint32_t cb_t = crossbars[i*crossbar_vol_num+j].send(base_address + (total_num + i) * 0x1000);
            if (crossbar_times < cb_t) {
                crossbar_times = cb_t;
            }
        }
        uint32_t acc_t = accumulators[i].send(base_address + (total_num + crossbar_row_num + i) * 0x1000);
        uint32_t act_t = activations[i].send(target_address);
        
        if (acc_times < acc_t) {
            acc_times = acc_t;
        }
        if (act_times < act_t) {
            act_times = act_t;
        }
    }
    this->times += (crossbar_times + acc_times + act_times);
}

PoolingLayer::PoolingLayer(uint32_t input_size[3], uint32_t kernel_size[3], uint32_t crossbar_size, Interconnect *ic, std::string type)
: NeuralNetworkLayer(crossbar_size, ic), _pool(crossbar_size, ic, type) {
    std::copy(input_size, input_size + 3, this->input_size);
    std::copy(kernel_size, kernel_size + 3, this->kernel_size);
    ic->registerComponent(&_pool);
}

std::vector<uint32_t> PoolingLayer::get_input_addr() {
    return std::vector<uint32_t>(1, _pool.getAddress());
}

void PoolingLayer::forward_propagation(std::vector<uint32_t> target_addresses) {
    _pool.pooling(input_size, kernel_size);
    if (target_addresses.size() > 1) {
        this->times += _pool.send(target_addresses);
    } else {
        this->times += _pool.send(target_addresses[0]);
    }
}

void PoolingLayer::forward_propagation(uint32_t target_address) {
    _pool.pooling(input_size, kernel_size);
    this->times += _pool.send(target_address);
}

FlattenLayer::FlattenLayer(uint32_t crossbar_size, Interconnect *ic)
: NeuralNetworkLayer(crossbar_size, ic), _flatten(crossbar_size, ic) {
    ic->registerComponent(&_flatten);
}

std::vector<uint32_t> FlattenLayer::get_input_addr() {
    // return _flatten.getAddress();
    return std::vector<uint32_t>(1, _flatten.getAddress());
}

void FlattenLayer::forward_propagation(std::vector<uint32_t> target_addresses) {
    this->times += _flatten.send(target_addresses);
}
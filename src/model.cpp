#include "model.hpp"


Model::Model(const std::array<uint32_t, 3>& input_size, uint32_t cs, Host* h, Interconnect* ic)
    : crossbar_size(cs), host(h), interconnect(ic) {
    current_size[0] = input_size[0];
    current_size[1] = input_size[1];
    current_size[2] = input_size[2];
    this->input_size[0] = input_size[0];
    this->input_size[1] = input_size[1];
    this->input_size[2] = input_size[2];
}

Model& Model::Conv(uint32_t kh, uint32_t kw, uint32_t filters, uint32_t stride, uint32_t pad, const std::string& act) {
    uint32_t kernel[3] = {kh, kw, filters};
    uint32_t output[3] = {
        (current_size[0] + 2 * pad - kh) / stride + 1,
        (current_size[1] + 2 * pad - kw) / stride + 1,
        filters
    };
    auto* conv = new ConvolutionLayer(current_size, kernel, stride, pad, crossbar_size, interconnect, act);
    layers.push_back(conv);

    std::copy(output, output + 3, current_size);
    return *this;
}

Model& Model::MaxPool(uint32_t ph, uint32_t pw) {
    uint32_t kernel[3] = {ph, pw, 1};
    uint32_t output[3] = {
        current_size[0] / ph,
        current_size[1] / pw,
        current_size[2]
    };
    auto* pool = new PoolingLayer(current_size, kernel, crossbar_size, interconnect, "Max");
    layers.push_back(pool);

    std::copy(output, output + 3, current_size);
    return *this;
}

Model& Model::Flatten() {
    auto* flatten = new FlattenLayer(crossbar_size, interconnect);
    layers.push_back(flatten);

    current_size[0] = 1;
    current_size[1] = 1;
    current_size[2] = current_size[0] * current_size[1] * current_size[2]; // flattens to 1D
    return *this;
}

Model& Model::Dense(uint32_t out_features, const std::string& act) {
    uint32_t in_features = current_size[0] * current_size[1] * current_size[2];
    auto* fc = new FullyConnectedLayer(in_features, out_features, crossbar_size, interconnect, act);
    layers.push_back(fc);

    current_size[2] = out_features;
    current_size[0] = current_size[1] = 1;
    return *this;
}

void Model::forward() {
    if (auto* conv = dynamic_cast<ConvolutionLayer*>(layers[0])) {
        this->delay += conv->set_up(host, input_size[0] * input_size[1]);
    } else if (auto* fc = dynamic_cast<FullyConnectedLayer*>(layers[0])) {
        this->delay += fc->set_up(host, input_size[0] * input_size[1]);
    } else {
        std::cerr << "Unknown component type for connection.\n";
    }

    for (size_t i = 1; i < layers.size(); ++i) {
        layers[i-1]->forward_propagation(layers[i]->get_input_addr());
        this->delay += layers[i-1]->get_delay();
    }

    // Final layer output goes to host
    layers.back()->forward_propagation(host->getAddress());
    this->delay += layers.back()->get_delay();
}

uint32_t Model::get_delay() { return delay; }

Model::~Model() {
    for (auto* c : layers)
        delete c;
}

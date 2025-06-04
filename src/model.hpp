#pragma once
#include "layers.hpp"

class Model {
    private:
        Interconnect* interconnect;
        Host* host;
        uint32_t crossbar_size;
        uint32_t input_size[3];
        uint32_t current_size[3];
        uint32_t delay = 0;
    
        std::vector<NeuralNetworkLayer*> layers;
    
    public:
        Model(const std::array<uint32_t, 3>& input_size, uint32_t cs, Host* h, Interconnect* ic);
    
        Model& Conv(uint32_t kh, uint32_t kw, uint32_t filters, uint32_t stride = 1, uint32_t pad = 0, const std::string& act = "relu");
    
        Model& MaxPool(uint32_t ph, uint32_t pw);
    
        Model& Flatten();
    
        Model& Dense(uint32_t out_features, const std::string& act = "relu");
    
        void forward();

        uint32_t get_delay();
    
        ~Model();
    };
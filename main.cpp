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
        Model(const std::array<uint32_t, 3>& input_size, uint32_t cs, Host* h, Interconnect* ic)
            : crossbar_size(cs), host(h), interconnect(ic) {
            current_size[0] = input_size[0];
            current_size[1] = input_size[1];
            current_size[2] = input_size[2];
            this->input_size[0] = input_size[0];
            this->input_size[1] = input_size[1];
            this->input_size[2] = input_size[2];
        }
    
        Model& Conv(uint32_t kh, uint32_t kw, uint32_t filters, uint32_t stride = 1, uint32_t pad = 0, const std::string& act = "relu") {
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
    
        Model& MaxPool(uint32_t ph, uint32_t pw) {
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
    
        Model& Flatten() {
            auto* flatten = new FlattenLayer(crossbar_size, interconnect);
            layers.push_back(flatten);
    
            current_size[0] = 1;
            current_size[1] = 1;
            current_size[2] = current_size[0] * current_size[1] * current_size[2]; // flattens to 1D
            return *this;
        }
    
        Model& Dense(uint32_t out_features, const std::string& act = "relu") {
            uint32_t in_features = current_size[0] * current_size[1] * current_size[2];
            auto* fc = new FullyConnectedLayer(in_features, out_features, crossbar_size, interconnect, act);
            layers.push_back(fc);

            current_size[2] = out_features;
            current_size[0] = current_size[1] = 1;
            return *this;
        }
    
        void forward() {
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

        uint32_t get_delay() { return delay; }
    
        ~Model() {
            for (auto* c : layers)
                delete c;
        }
    };

// Main Simulation
int main() {
    auto start = std::chrono::high_resolution_clock::now();

    std::string file_name = "network.dot";
    Interconnect interconnect(file_name);
    Host host = Host(64*1024*8, &interconnect);
    interconnect.registerComponent(&host);
    Model model({28, 28, 1}, CROSSBAR_SIZE, &host, &interconnect);

    model.Conv(3, 3, 32)
        .MaxPool(2, 2)
        .Conv(3, 3, 64)
        .MaxPool(2, 2)
        .Conv(3, 3, 64)
        .Flatten()
        .Dense(64)
        .Dense(10);
    
    // model.Dense(512)
    //     .Dense(64)
    //     .Dense(10);

    model.forward();

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    std::ofstream dotFile;
    dotFile.open("report.txt");
    dotFile << "Crossbar Size: " << CROSSBAR_SIZE << "*" << CROSSBAR_SIZE << "\n"
    << "Bit Precision: " << BIT_PRECISION << "\n"
    << "Crossbar Amount: " << interconnect.getCrossbarNum() << "\n"
    << "Crossbar Usage Proportion: " << interconnect.getCrossbarUsage() << "\n"
    << "Required Minimum Bandwidth: " << interconnect.getMinBandwidth() << " bits per unit time\n"
    << "Delay: " << model.get_delay() << " unit time\n"
    << "Total Bits transferred: " << interconnect.getTotalBits() << " bits\n\n"
    << "Sim Time Cost: " << duration.count() << "e-6 s\n";
    dotFile.close();
    return 0;
}

// bool nn_flag = false;
    // if (nn_flag) {
    //     FullyConnectedLayer hidden_layer_0 = FullyConnectedLayer(784, 512, CROSSBAR_SIZE, &interconnect, "relu"); // 784*512
    //     FullyConnectedLayer hidden_layer_1 = FullyConnectedLayer(512, 32, CROSSBAR_SIZE, &interconnect, "relu"); // 512*32
    //     FullyConnectedLayer output_layer = FullyConnectedLayer(32, 10, CROSSBAR_SIZE, &interconnect, "softmax"); // 32*10

    //     hidden_layer_0.set_up(&host, 28*28);
    //     hidden_layer_0.forward_propagation(hidden_layer_1.get_input_addr());
    //     hidden_layer_1.forward_propagation(output_layer.get_input_addr());
    //     output_layer.forward_propagation(host.getAddress());
    //     std::cout << "***" << hidden_layer_0.get_delay() << std::endl;
    // } else {
    //     uint32_t stride = 1;
    //     uint32_t pad = 0;
    //     uint32_t input_sz_0[3] = {28, 28, 1};
    //     uint32_t kernel_sz_0[3] = {3, 3, 32};
    //     ConvolutionLayer hidden_layer_0 = ConvolutionLayer(input_sz_0, kernel_sz_0, stride, pad, CROSSBAR_SIZE, &interconnect, "relu");
    //     uint32_t pool_input_sz_0[3] = {(26 + pad) / stride, (26 + pad) / stride, 32};
    //     uint32_t pool_kernel_sz_0[3] = {2, 2, 1};
    //     PoolingLayer pooling_layer_0 = PoolingLayer(pool_input_sz_0, pool_kernel_sz_0, CROSSBAR_SIZE, &interconnect, "Max");

    //     uint32_t input_sz_1[3] = {13, 13, 32};
    //     uint32_t kernel_sz_1[3] = {3, 3, 64};
    //     ConvolutionLayer hidden_layer_1 = ConvolutionLayer(input_sz_1, kernel_sz_1, stride, pad, CROSSBAR_SIZE, &interconnect, "relu");
    //     uint32_t pool_input_sz_1[3] = {(11 + pad) / stride, (11 + pad) / stride, 64};
    //     uint32_t pool_kernel_sz_1[3] = {2, 2, 1};
    //     PoolingLayer pooling_layer_1 = PoolingLayer(pool_input_sz_1, pool_kernel_sz_1, CROSSBAR_SIZE, &interconnect, "Max");

    //     uint32_t input_sz_2[3] = {5, 5, 64};
    //     uint32_t kernel_sz_2[3] = {3, 3, 64};
    //     ConvolutionLayer hidden_layer_2 = ConvolutionLayer(input_sz_2, kernel_sz_2, stride, pad, CROSSBAR_SIZE, &interconnect, "relu");

    //     FlattenLayer flattenlayer = FlattenLayer(CROSSBAR_SIZE, &interconnect);
    //     FullyConnectedLayer hidden_layer_3 = FullyConnectedLayer(576, 64, CROSSBAR_SIZE, &interconnect, "relu"); // 576*64
    //     FullyConnectedLayer output_layer = FullyConnectedLayer(64, 10, CROSSBAR_SIZE, &interconnect, "relu"); // 64*10

    //     hidden_layer_0.set_up(&host, 28*28);
    //     hidden_layer_0.forward_propagation(pooling_layer_0.get_input_addr());
    //     pooling_layer_0.forward_propagation(hidden_layer_1.get_input_addr());
    //     hidden_layer_1.forward_propagation(pooling_layer_1.get_input_addr());
    //     pooling_layer_1.forward_propagation(hidden_layer_2.get_input_addr());
    //     hidden_layer_2.forward_propagation(flattenlayer.get_input_addr());
    //     flattenlayer.forward_propagation(hidden_layer_3.get_input_addr());
    //     hidden_layer_3.forward_propagation(output_layer.get_input_addr());
    //     output_layer.forward_propagation(host.getAddress());
    //     std::cout << "***" << hidden_layer_0.get_delay() << std::endl;
    //     std::cout << "***" << pooling_layer_0.get_delay() << std::endl;
    //     std::cout << "***" << hidden_layer_1.get_delay() << std::endl;
    //     std::cout << "***" << pooling_layer_1.get_delay() << std::endl;
    //     std::cout << "***" << hidden_layer_2.get_delay() << std::endl;
    //     std::cout << "***" << flattenlayer.get_delay() << std::endl;
    //     std::cout << "***" << hidden_layer_3.get_delay() << std::endl;
    //     std::cout << "***" << output_layer.get_delay() << std::endl;
    // }
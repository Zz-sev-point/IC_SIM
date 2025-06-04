#include "model.hpp"
#include <sstream>

// Main Simulation
int main() {
    auto start = std::chrono::high_resolution_clock::now();

    std::string file_name = "network.dot";
    Interconnect interconnect(file_name);
    Host host = Host(64*1024*8, &interconnect);
    interconnect.registerComponent(&host);
    Model model({28, 28, 1}, CROSSBAR_SIZE, &host, &interconnect);

    model.Conv(3, 3, 64)
         .MaxPool(2, 2)
         .Conv(3, 3, 64)
         .Flatten()
         .Dense(64)
         .Dense(10);
    
    // model.Dense(512)
    //     .Dense(64)
    //     .Dense(10);

    //model.Dense(128)
    //.Dense(10);

    model.forward();

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    // Construct filename based on parameters
    std::stringstream filenameStream;
    filenameStream << "./cnn-k2col/" << CROSSBAR_SIZE << "-" << BIT_PRECISION << ".txt";
    std::string filename = filenameStream.str();

    std::ofstream dotFile;
    // dotFile.open(filename);
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
    // // if (nn_flag) {
    //     FullyConnectedLayer hidden_layer_0 = FullyConnectedLayer(784, 512, CROSSBAR_SIZE, &interconnect, "relu"); // 784*512
    //     FullyConnectedLayer hidden_layer_1 = FullyConnectedLayer(512, 32, CROSSBAR_SIZE, &interconnect, "relu"); // 512*32
    //     FullyConnectedLayer output_layer = FullyConnectedLayer(32, 10, CROSSBAR_SIZE, &interconnect, "softmax"); // 32*10

    //     hidden_layer_0.set_up(&host, 28*28);
    //     hidden_layer_0.forward_propagation(hidden_layer_1.get_input_addr());
    //     hidden_layer_1.forward_propagation(output_layer.get_input_addr());
    //     output_layer.forward_propagation(host.getAddress());
    //     std::cout << "***" << hidden_layer_0.get_delay() << std::endl;
    // } else {
        // uint32_t stride = 1;
        // uint32_t pad = 0;
        // uint32_t input_sz_0[3] = {28, 28, 1};
        // uint32_t kernel_sz_0[3] = {3, 3, 32};
        // ConvolutionLayer hidden_layer_0 = ConvolutionLayer(input_sz_0, kernel_sz_0, stride, pad, CROSSBAR_SIZE, &interconnect, "relu");
        // uint32_t pool_input_sz_0[3] = {(26 + pad) / stride, (26 + pad) / stride, 32};
        // uint32_t pool_kernel_sz_0[3] = {2, 2, 1};
        // PoolingLayer pooling_layer_0 = PoolingLayer(pool_input_sz_0, pool_kernel_sz_0, CROSSBAR_SIZE, &interconnect, "Max");

        // uint32_t input_sz_1[3] = {13, 13, 32};
        // uint32_t kernel_sz_1[3] = {3, 3, 64};
        // ConvolutionLayer hidden_layer_1 = ConvolutionLayer(input_sz_1, kernel_sz_1, stride, pad, CROSSBAR_SIZE, &interconnect, "relu");
        // uint32_t pool_input_sz_1[3] = {(11 + pad) / stride, (11 + pad) / stride, 64};
        // uint32_t pool_kernel_sz_1[3] = {2, 2, 1};
        // PoolingLayer pooling_layer_1 = PoolingLayer(pool_input_sz_1, pool_kernel_sz_1, CROSSBAR_SIZE, &interconnect, "Max");

        // uint32_t input_sz_2[3] = {5, 5, 64};
        // uint32_t kernel_sz_2[3] = {3, 3, 64};
        // ConvolutionLayer hidden_layer_2 = ConvolutionLayer(input_sz_2, kernel_sz_2, stride, pad, CROSSBAR_SIZE, &interconnect, "relu");

        // FlattenLayer flattenlayer = FlattenLayer(CROSSBAR_SIZE, &interconnect);
        // FullyConnectedLayer hidden_layer_3 = FullyConnectedLayer(576, 64, CROSSBAR_SIZE, &interconnect, "relu"); // 576*64
        // FullyConnectedLayer output_layer = FullyConnectedLayer(64, 10, CROSSBAR_SIZE, &interconnect, "relu"); // 64*10

        // hidden_layer_0.set_up(&host, 28*28);
        // hidden_layer_0.forward_propagation(pooling_layer_0.get_input_addr());
        // pooling_layer_0.forward_propagation(hidden_layer_1.get_input_addr());
        // hidden_layer_1.forward_propagation(pooling_layer_1.get_input_addr());
        // pooling_layer_1.forward_propagation(hidden_layer_2.get_input_addr());
        // hidden_layer_2.forward_propagation(flattenlayer.get_input_addr());
        // flattenlayer.forward_propagation(hidden_layer_3.get_input_addr());
        // hidden_layer_3.forward_propagation(output_layer.get_input_addr());
        // output_layer.forward_propagation(host.getAddress());
    //     std::cout << "***" << hidden_layer_0.get_delay() << std::endl;
    //     std::cout << "***" << pooling_layer_0.get_delay() << std::endl;
    //     std::cout << "***" << hidden_layer_1.get_delay() << std::endl;
    //     std::cout << "***" << pooling_layer_1.get_delay() << std::endl;
    //     std::cout << "***" << hidden_layer_2.get_delay() << std::endl;
    //     std::cout << "***" << flattenlayer.get_delay() << std::endl;
    //     std::cout << "***" << hidden_layer_3.get_delay() << std::endl;
    //     std::cout << "***" << output_layer.get_delay() << std::endl;
    // }
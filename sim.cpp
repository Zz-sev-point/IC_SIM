#include <iostream>
#include <vector>
#include <queue>
#include <random>
#include <chrono>
#include <thread>
#include <bitset>

constexpr int CROSSBAR_ROWS = 512;  // Fixed output size per crossbar
constexpr int CROSSBAR_COLS = 512;  // Fixed input size per crossbar
constexpr int LATENCY = 2;     // Base latency for each transfer
constexpr int INPUT_BIT_WIDTH = 8;  // 8-bit inputs
constexpr int WEIGHT_BIT_WIDTH = 4; 
constexpr int HIDDEN_LAYER_NEURAL_AMOUNT = 128;
constexpr int LABEL_AMOUNT = 10;

class Crossbar {
public:
    int id;
    std::vector<std::vector<int>> weights; 

    Crossbar(int id) : id(id) {
        weights.resize(CROSSBAR_ROWS, std::vector<int>(CROSSBAR_COLS));
        randomizeWeights();
    }

    void randomizeWeights() {
        for (int i = 0; i < CROSSBAR_ROWS; ++i) {
            for (int j = 0; j < CROSSBAR_COLS; ++j) {
                weights[i][j] = rand() % 2; 
            }
        }
    }

    std::vector<int> compute(const std::vector<int>& input) {
        std::vector<int> output(CROSSBAR_ROWS, 0);

        for (int i = 0; i < CROSSBAR_ROWS; ++i) {
            int acc = 0;
            for (int j = 0; j < std::min(CROSSBAR_COLS, (int)input.size()); ++j) {
                int weight_bits = weights[i][j];
                std::bitset<INPUT_BIT_WIDTH> input_bits = input[j];

                int partial_sum = 0;
                
                // Multiply weight by each input bit
                for (int x_bit = 0; x_bit < INPUT_BIT_WIDTH; ++x_bit) {
                    if (weight_bits==1 && input_bits[x_bit]) {
                        partial_sum += (1 << x_bit); // Shifted contribution
                    }
                }

                acc += partial_sum; // Accumulate row results
            }
            output[i] = acc;
        }

        return output;
    }
};

class NeuralNetwork {
private:
    std::vector<Crossbar> crossbars;
    int numLayers;
    std::vector<int> numInputs;
    std::vector<int> numNeurals;
    std::vector<int> numCrossbars;

    std::vector<std::vector<int>> splitIntoTiles(const std::vector<int>& input) {
        std::vector<std::vector<int>> tiles;
        for (size_t i = 0; i < input.size(); i += CROSSBAR_COLS) {
            std::vector<int> tile(input.begin() + i, input.begin() + std::min(i + CROSSBAR_COLS, input.size()));
            if (tile.size() < CROSSBAR_COLS) tile.resize(CROSSBAR_COLS, 0.0);  // Zero padding
            tiles.push_back(tile);
        }
        return tiles;
    }

    std::vector<int> mergeTiles(const std::vector<std::vector<int>>& tiles) {
        std::vector<int> merged = std::vector(CROSSBAR_ROWS, 0);
        for (const auto& tile : tiles) {
            for (size_t i = 0; i < merged.size(); ++i) {
                merged[i] += tile[i];
            }
        }
        return merged;
    }

    std::vector<int> connect(const std::vector<std::vector<int>>& tiles) {
        std::vector<int> connected = std::vector(CROSSBAR_ROWS, 0);
        for (const auto& tile : tiles) {
            connected.insert(connected.end(), tile.begin(), tile.end());
        }
        return connected;
    }

public:
    NeuralNetwork(int numLayers, std::vector<int> numInputs, std::vector<int> numNeurals) : numLayers(numLayers), numInputs(numInputs), numNeurals(numNeurals) {
        int acc = 0;
        numCrossbars.push_back(acc);
        for (size_t i = 0; i < numLayers; ++i) {
            for (int j = 0; j < ((WEIGHT_BIT_WIDTH*numNeurals[i]+CROSSBAR_ROWS-1)/CROSSBAR_ROWS)*((numInputs[i]+CROSSBAR_COLS-1)/CROSSBAR_COLS); ++j) {
                crossbars.emplace_back(acc);
                acc++;
            }
            numCrossbars.push_back(acc);
        }
    }

    void simulateTraffic(std::vector<int> input) {
        std::vector<int> temp;
        int bits_weight = 1;
        for (size_t i = 0; i < numLayers; ++i) {
            std::cout << "** Layer " << i+1 << " **" << std::endl;
            std::vector<std::vector<int>> outputs;

            for (size_t j = numCrossbars[i]; j < numCrossbars[i+1]; ++j) {
                // if (numCrossbars[i] == 0) {
                    std::cout << "[Network] Uploading " << numInputs[i]*bits_weight*INPUT_BIT_WIDTH << " bits data to Crossbar " << j << "..." << std::endl;
                // } else {
                //     std::cout << "[Network] Sending data from Crossbar " << numCrossbars[i-1] << " to Crossbar " << j << std::endl;
                // }
                std::cout << "[Compute] Processing on Crossbar " << j << "..." << std::endl;
                std::vector<std::vector<int>> inputTiles = splitIntoTiles(input);
                std::vector<std::vector<int>> outputTiles;

                for (const auto& tile : inputTiles) {
                    std::vector<int> out = crossbars[j].compute(tile);
                    outputTiles.push_back(out);
                }
                outputs.push_back(mergeTiles(outputTiles));
                if (j != numCrossbars[i]) {
                    std::cout << "[Network] Sending data from Crossbar " << j << " to Crossbar " << numCrossbars[i] << std::endl;
                }
            }
            std::cout << "[Network] Sending data from Crossbar " << numCrossbars[i] << " to the Host " << std::endl;
            
            input = connect(outputs);
            bits_weight *= WEIGHT_BIT_WIDTH;


            // if (i < crossbars.size() - 1) {
            //     network.sendData(numCrossbars[i], numCrossbars[i+1], input);
            //     input = network.receiveData(numCrossbars[i+1]);
            //     while (input.empty()) {
            //         std::this_thread::sleep_for(std::chrono::milliseconds(50));
            //         input = network.receiveData(i + 1);
            //     }
            // }
        }

        std::cout << "[Result] Final output: ";
        for (size_t i = 0; i < LABEL_AMOUNT; ++i) std::cout << input[i] << " ";
        std::cout << std::endl;
    }
};

int main() {
    int layers = 2;
    std::vector<int> neurals = {128, 10};
    std::vector<int> inputs = {784, 128};
    NeuralNetwork nn(layers, inputs, neurals);

    std::vector<int> input = std::vector(784, 0);
    // Fill with random numbers
    for (int& v : input) {
        v = rand() % 256; // Random values between 0-255
    }
    nn.simulateTraffic(input);

    return 0;
}
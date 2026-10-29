#include <iostream>
#include <vector>
#include "function.h"

int main() {
    const int width = 64;
    const int height = 64;
    const int channels = 3;

    // Allocate simulated RGB input image
    uint8_t* raw_input = new uint8_t[width * height * channels];
    for (int i = 0; i < width * height * channels; ++i) {
        raw_input[i] = static_cast<uint8_t>(i % 256);
    }

    Image input{width, height, channels, raw_input};

    std::cout << "Running image processing pipeline..." << std::endl;
    Image* output = process_image(&input);

    if (output && output->data) {
        std::cout << "Pipeline completed successfully!" << std::endl;
        std::cout << "Output: " << output->width << "x" << output->height 
                  << " (" << output->channels << " channels)" << std::endl;
        
        // Manual cleanup required by caller
        delete[] output->data;
        delete output;
    } else {
        std::cerr << "Pipeline failed!" << std::endl;
    }

    delete[] raw_input;
    return 0;
}

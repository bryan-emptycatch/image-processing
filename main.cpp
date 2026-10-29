#include <iostream>
#include <chrono>
#include <iomanip>
#include <cstdlib>
#include "function.h"

int main(int argc, char* argv[]) {
    // Default to 4K UHD resolution: 3840 x 2160 (~24.9 MB per RGB frame)
    int width = 3840;
    int height = 2160;
    int iterations = 10;

    if (argc >= 3) {
        width = std::atoi(argv[1]);
        height = std::atoi(argv[2]);
    }
    if (argc >= 4) {
        iterations = std::atoi(argv[3]);
    }

    const int channels = 3;
    const size_t num_pixels = static_cast<size_t>(width) * height;
    const size_t buffer_size = num_pixels * channels;
    const double mb_per_image = static_cast<double>(buffer_size) / (1024.0 * 1024.0);

    std::cout << "========================================\n";
    std::cout << "Image Processing Memory Benchmark\n";
    std::cout << "Resolution:   " << width << " x " << height 
              << " (" << std::fixed << std::setprecision(2) << (num_pixels / 1'000'000.0) << " MP)\n";
    std::cout << "Raw RGB Size: " << mb_per_image << " MB per frame\n";
    std::cout << "Iterations:   " << iterations << "\n";
    std::cout << "========================================\n";

    std::cout << "Allocating input frame (" << mb_per_image << " MB)..." << std::endl;
    uint8_t* raw_input = new uint8_t[buffer_size];
    for (size_t i = 0; i < buffer_size; ++i) {
        raw_input[i] = static_cast<uint8_t>(i % 256);
    }

    Image input{width, height, channels, raw_input};

    std::cout << "Running pipeline across " << iterations << " iterations..." << std::endl;
    auto start = std::chrono::steady_clock::now();

    for (int it = 0; it < iterations; ++it) {
        Image* output = process_image(&input);
        if (!output || !output->data) {
            std::cerr << "Pipeline failed on iteration " << it << std::endl;
            delete[] raw_input;
            return 1;
        }

        // In high-throughput pipelines, manual deallocation is critical;
        // any missed delete leaks ~8.3 MB per iteration!
        delete[] output->data;
        delete output;
    }

    auto end = std::chrono::steady_clock::now();
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    double avg_ms = static_cast<double>(elapsed_ms) / iterations;
    double total_sec = elapsed_ms > 0 ? (elapsed_ms / 1000.0) : 0.001;
    double throughput_mp = (num_pixels * iterations / 1'000'000.0) / total_sec;

    std::cout << "\nBenchmark complete!\n";
    std::cout << "Total time:   " << elapsed_ms << " ms\n";
    std::cout << "Average time: " << avg_ms << " ms/frame\n";
    std::cout << "Throughput:   " << throughput_mp << " MP/sec\n";

    delete[] raw_input;
    return 0;
}

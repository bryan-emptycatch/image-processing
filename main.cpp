#include <iostream>
#include <chrono>
#include <iomanip>
#include <cstdlib>
#include <atomic>
#include <cstddef>
#include "function.h"

// =========================================================================
// Real-Time Heap Memory Tracker
// Intercepts global new/delete to track active heap bytes and detect leaks.
// =========================================================================
struct MemoryTracker {
    std::atomic<size_t> current_bytes{0};
    std::atomic<size_t> peak_bytes{0};
    std::atomic<size_t> total_allocated{0};
    std::atomic<size_t> alloc_count{0};
    std::atomic<size_t> free_count{0};

    void record_alloc(size_t size) {
        current_bytes += size;
        total_allocated += size;
        alloc_count++;
        size_t cur = current_bytes.load();
        size_t peak = peak_bytes.load();
        while (cur > peak && !peak_bytes.compare_exchange_weak(peak, cur));
    }

    void record_dealloc(size_t size) {
        current_bytes -= size;
        free_count++;
    }
};

static MemoryTracker g_mem_tracker;

// 16-byte alignment to preserve std::max_align_t alignment for SIMD/vector operations
constexpr size_t HEADER_SIZE = sizeof(std::max_align_t);

void* operator new(size_t size) {
    void* ptr = std::malloc(size + HEADER_SIZE);
    if (!ptr) throw std::bad_alloc();
    *reinterpret_cast<size_t*>(ptr) = size;
    g_mem_tracker.record_alloc(size);
    return static_cast<char*>(ptr) + HEADER_SIZE;
}

void operator delete(void* ptr) noexcept {
    if (!ptr) return;
    void* orig = static_cast<char*>(ptr) - HEADER_SIZE;
    size_t size = *reinterpret_cast<size_t*>(orig);
    g_mem_tracker.record_dealloc(size);
    std::free(orig);
}

void operator delete(void* ptr, size_t) noexcept { operator delete(ptr); }
void* operator new[](size_t size) { return operator new(size); }
void operator delete[](void* ptr) noexcept { operator delete(ptr); }
void operator delete[](void* ptr, size_t) noexcept { operator delete(ptr); }

inline double to_mb(size_t bytes) {
    return static_cast<double>(bytes) / (1024.0 * 1024.0);
}

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

    std::cout << "=====================================================\n";
    std::cout << "Image Processing Memory & Performance Benchmark\n";
    std::cout << "Resolution:        " << width << " x " << height 
              << " (" << std::fixed << std::setprecision(2) << (num_pixels / 1'000'000.0) << " MP)\n";
    std::cout << "Expected Frame:    " << to_mb(buffer_size) << " MB (Raw RGB)\n";
    std::cout << "Iterations:        " << iterations << "\n";
    std::cout << "=====================================================\n";

    std::cout << "[Step 0] Initial Heap Usage:           " 
              << to_mb(g_mem_tracker.current_bytes.load()) << " MB\n";

    std::cout << "[Step 1] Allocating input image buffer...\n";
    uint8_t* raw_input = new uint8_t[buffer_size];
    for (size_t i = 0; i < buffer_size; ++i) {
        raw_input[i] = static_cast<uint8_t>(i % 256);
    }
    std::cout << "         Heap Usage after input alloc: " 
              << to_mb(g_mem_tracker.current_bytes.load()) << " MB\n";

    Image input{width, height, channels, raw_input};

    std::cout << "[Step 2] Executing pipeline across " << iterations << " iterations...\n";
    auto start = std::chrono::steady_clock::now();

    for (int it = 0; it < iterations; ++it) {
        Image* output = process_image(&input);
        if (!output || !output->data) {
            std::cerr << "Pipeline failed on iteration " << it << std::endl;
            delete[] raw_input;
            return 1;
        }

        if (it == 0) {
            std::cout << "         Heap Usage (1st iter with output held):  " 
                      << to_mb(g_mem_tracker.current_bytes.load()) << " MB\n";
        }

        // Cleanup caller-owned result
        delete[] output->data;
        delete output;

        if (it == 0) {
            std::cout << "         Heap Usage (1st iter after output freed): " 
                      << to_mb(g_mem_tracker.current_bytes.load()) << " MB\n";
        }
    }

    auto end = std::chrono::steady_clock::now();
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    double avg_ms = static_cast<double>(elapsed_ms) / iterations;
    double total_sec = elapsed_ms > 0 ? (elapsed_ms / 1000.0) : 0.001;
    double throughput_mp = (num_pixels * iterations / 1'000'000.0) / total_sec;

    std::cout << "[Step 3] Deleting input frame...\n";
    delete[] raw_input;
    std::cout << "         Heap Usage after input freed: " 
              << to_mb(g_mem_tracker.current_bytes.load()) << " MB\n";

    std::cout << "\n=====================================================\n";
    std::cout << "Benchmark Results & Diagnostics\n";
    std::cout << "=====================================================\n";
    std::cout << "Total Runtime:       " << elapsed_ms << " ms (" << avg_ms << " ms/frame)\n";
    std::cout << "Throughput:          " << throughput_mp << " MP/sec\n";
    std::cout << "Peak Active Memory:  " << to_mb(g_mem_tracker.peak_bytes.load()) << " MB\n";
    std::cout << "Total Data Allocated:" << to_mb(g_mem_tracker.total_allocated.load()) << " MB\n";
    std::cout << "Total Allocations:   " << g_mem_tracker.alloc_count.load() << "\n";
    std::cout << "Total Deallocations: " << g_mem_tracker.free_count.load() << "\n";

    size_t leaked_bytes = g_mem_tracker.current_bytes.load();
    if (leaked_bytes == 0) {
        std::cout << "Leak Check Status:   PASSED (0 bytes leaked)\n";
    } else {
        std::cout << "Leak Check Status:   FAILED! (" << to_mb(leaked_bytes) << " MB leaked, " 
                  << leaked_bytes << " bytes)\n";
    }
    std::cout << "=====================================================\n";

    return leaked_bytes == 0 ? 0 : 1;
}

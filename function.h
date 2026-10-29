#pragma once
#include <cstdint>
#include <memory>

struct Image {
    int width;
    int height;
    int channels;
    uint8_t* data;
};

// Pipeline stage helpers refactored to RAII
std::unique_ptr<uint8_t[]> extract_luminance(const Image* input);
std::unique_ptr<uint8_t[]> boost_brightness(const uint8_t* buffer, int count, int delta);
std::unique_ptr<uint8_t[]> apply_horizontal_blur(const uint8_t* buffer, int width, int height);
Image* make_image(uint8_t* buffer, int width, int height, int channels);

// Primary pipeline entrypoint
Image* process_image(const Image* input);

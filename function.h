#pragma once
#include <cstdint>

struct Image {
    int width;
    int height;
    int channels;
    uint8_t* data;
};

// Pipeline stage helpers
uint8_t* extract_luminance(const Image* input);
uint8_t* boost_brightness(const uint8_t* buffer, int count, int delta);
uint8_t* apply_horizontal_blur(const uint8_t* buffer, int width, int height);
Image* make_image(uint8_t* buffer, int width, int height, int channels);

// Primary pipeline entrypoint
Image* process_image(const Image* input);

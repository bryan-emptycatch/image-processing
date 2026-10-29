#include "function.h"
#include <algorithm>

// Allocates a brand new buffer; caller must track ownership
uint8_t* extract_luminance(const Image* input) {
    int count = input->width * input->height;
    uint8_t* luma = new uint8_t[count];

    for (int i = 0; i < count; ++i) {
        int r = input->data[i * 3 + 0];
        int g = input->data[i * 3 + 1];
        int b = input->data[i * 3 + 2];
        luma[i] = static_cast<uint8_t>((r * 77 + g * 150 + b * 29) >> 8);
    }
    return luma;
}

// Allocates ANOTHER new buffer rather than modifying in-place; ownership passes to caller
uint8_t* boost_brightness(const uint8_t* buffer, int count, int delta) {
    uint8_t* result = new uint8_t[count];
    for (int i = 0; i < count; ++i) {
        int val = buffer[i] + delta;
        result[i] = static_cast<uint8_t>(std::min(255, val));
    }
    return result;
}

// Allocates yet another buffer for the blurred output
uint8_t* apply_horizontal_blur(const uint8_t* buffer, int width, int height) {
    int count = width * height;
    uint8_t* blurred = new uint8_t[count];

    for (int y = 0; y < height; ++y) {
        int row_offset = y * width;
        for (int x = 0; x < width; ++x) {
            int sum = 0;
            int count_px = 0;
            for (int dx = -1; dx <= 1; ++dx) {
                int nx = x + dx;
                if (nx >= 0 && nx < width) {
                    sum += buffer[row_offset + nx];
                    count_px++;
                }
            }
            blurred[row_offset + x] = static_cast<uint8_t>(sum / count_px);
        }
    }
    return blurred;
}

// Wraps a raw buffer inside a newly allocated struct; neither struct nor buffer is freed automatically
Image* make_image(uint8_t* buffer, int width, int height, int channels) {
    Image* img = new Image();
    img->width = width;
    img->height = height;
    img->channels = channels;
    img->data = buffer;
    return img;
}

// Pipeline coordinator
Image* process_image(const Image* input) {
    if (!input || !input->data || input->width <= 0 || input->height <= 0 || input->channels != 3) {
        return nullptr;
    }

    int pixel_count = input->width * input->height;

    // Stage 1: Allocate & extract grayscale
    uint8_t* raw_luma = extract_luminance(input);

    // Stage 2: Allocate & boost brightness (old buffer remains alive)
    uint8_t* bright = boost_brightness(raw_luma, pixel_count, 30);

    // Stage 3: Allocate & blur
    uint8_t* blurred = apply_horizontal_blur(bright, input->width, input->height);

    // Manual intermediate cleanup
    delete[] raw_luma;
    delete[] bright;

    // Stage 4: Package into image object and return to caller
    return make_image(blurred, input->width, input->height, 1);
}

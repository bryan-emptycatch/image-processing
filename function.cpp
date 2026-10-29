#include "function.h"
#include <algorithm>
#include <memory>

// Pipeline stage helpers refactored to return managed std::unique_ptr buffers
std::unique_ptr<uint8_t[]> extract_luminance(const Image* input) {
    int count = input->width * input->height;
    auto luma = std::make_unique<uint8_t[]>(count);

    for (int i = 0; i < count; ++i) {
        int r = input->data[i * 3 + 0];
        int g = input->data[i * 3 + 1];
        int b = input->data[i * 3 + 2];
        luma[i] = static_cast<uint8_t>((r * 77 + g * 150 + b * 29) >> 8);
    }
    return luma;
}

std::unique_ptr<uint8_t[]> boost_brightness(const uint8_t* buffer, int count, int delta) {
    auto result = std::make_unique<uint8_t[]>(count);
    for (int i = 0; i < count; ++i) {
        int val = buffer[i] + delta;
        result[i] = static_cast<uint8_t>(std::min(255, val));
    }
    return result;
}

std::unique_ptr<uint8_t[]> apply_horizontal_blur(const uint8_t* buffer, int width, int height) {
    int count = width * height;
    auto blurred = std::make_unique<uint8_t[]>(count);

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

Image* make_image(uint8_t* buffer, int width, int height, int channels) {
    auto img = std::make_unique<Image>();
    img->width = width;
    img->height = height;
    img->channels = channels;
    img->data = buffer;
    return img.release();
}

// Pipeline coordinator refactored to RAII
Image* process_image(const Image* input) {
    if (!input || !input->data || input->width <= 0 || input->height <= 0 || input->channels != 3) {
        return nullptr;
    }

    int pixel_count = input->width * input->height;

    // Stage 1: Allocate & extract grayscale via unique_ptr
    auto raw_luma = extract_luminance(input);

    // Stage 2: Allocate & boost brightness via unique_ptr
    auto bright = boost_brightness(raw_luma.get(), pixel_count, 30);

    // Stage 3: Allocate & blur via unique_ptr
    auto blurred = apply_horizontal_blur(bright.get(), input->width, input->height);

    // RAII handles destruction of raw_luma and bright automatically!
    // No manual delete[] needed.

    // Stage 4: Package into image object and transfer buffer ownership to Image container
    return make_image(blurred.release(), input->width, input->height, 1);
}

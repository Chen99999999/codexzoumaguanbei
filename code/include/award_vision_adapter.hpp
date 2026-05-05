#pragma once

#include <stdint.h>

#include "award_signal.hpp"

typedef unsigned char uchar;

struct AwardGrayImage
{
    int rows;
    int cols;
    uint8_t pixels[60][80];

    AwardGrayImage() : rows(60), cols(80), pixels{} {}

    template <typename T>
    T at(int row, int col) const
    {
        return static_cast<T>(pixels[row][col]);
    }
};

extern AwardGrayImage Gray_image;

void award_vision_init(void);
bool award_vision_process_from_gray(const uint8_t *gray, int width, int height, AwardSignal *signal);

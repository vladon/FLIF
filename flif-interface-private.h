#pragma once

#include "flif.h"

#include "image/image.hpp"
#include "flif-dec.hpp"
#include "flif-enc.hpp"
#include "fileio.hpp"

struct FLIF_IMAGE
{
    FLIF_IMAGE();

    void write_row_RGBA8(uint32_t row, const void* buffer, size_t buffer_size_bytes);
    void read_row_RGBA8(uint32_t row, void* buffer, size_t buffer_size_bytes);

    Image image;
};

struct FLIF_DECODER
{
    FLIF_DECODER();

    int32_t decode_file(const char* filename);
    int32_t decode_memory(const void* buffer, size_t buffer_size_bytes);
    size_t num_images();
    FLIF_IMAGE* get_image(size_t index);

    int32_t quality;
    uint32_t scale;
    void* callback;

private:
    Images internal_images;
    Images images;
//    std::vector<std::unique_ptr<FLIF_IMAGE>> requested_images;
};

struct FLIF_ENCODER
{
    FLIF_ENCODER();

    void add_image(FLIF_IMAGE* image);
    int32_t encode_file(const char* filename);
    int32_t encode_memory(void** buffer, size_t* buffer_size_bytes);

    uint32_t interlaced;
    uint32_t learn_repeats;
    uint32_t acb;
    uint32_t frame_delay;
    int32_t palette_size;
    int32_t lookback;
    int32_t divisor;
    int32_t min_size;
    int32_t split_threshold;

private:
    std::vector<FLIF_IMAGE*> images;
};

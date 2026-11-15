#pragma once
#include <stddef.h>
#include <stdint.h>

/* NVN BC texture format values from dm/gfx/nvn/include/nvn.h. */
enum {
    NVN_BC_DXT1_RGB = 0x41,
    NVN_BC_DXT1_RGBA = 0x42,
    NVN_BC_DXT3 = 0x43,
    NVN_BC_DXT5 = 0x44,
    NVN_BC_DXT1_RGB_SRGB = 0x45,
    NVN_BC_DXT1_RGBA_SRGB = 0x46,
    NVN_BC_DXT3_SRGB = 0x47,
    NVN_BC_DXT5_SRGB = 0x48,
    NVN_BC4_UNORM = 0x49,
    NVN_BC4_SNORM = 0x4A,
    NVN_BC5_UNORM = 0x4B,
    NVN_BC5_SNORM = 0x4C,
    NVN_BC7_UNORM = 0x4D,
    NVN_BC7_SRGB = 0x4E,
    NVN_BC6H_SFLOAT = 0x4F,
    NVN_BC6H_UFLOAT = 0x50
};

typedef enum {
    NVN_BC_OUTPUT_RGBA8,
    NVN_BC_OUTPUT_R8,
    NVN_BC_OUTPUT_RG8,
    NVN_BC_OUTPUT_R32_FLOAT,
    NVN_BC_OUTPUT_RG32_FLOAT,
    NVN_BC_OUTPUT_RGBA16_FLOAT
} nvn_bc_output;

/* Returns 0 on success, -1 for an unsupported format or invalid buffer/size.
   Source is a tightly packed sequence of 4x4 BC blocks, in row-major order.
   Destination is tightly packed by pixels with caller-supplied row pitch.
   BC6H expands RGB half-floats to RGBA16F with alpha = 1.0. */
int nvn_bc_decode_level(int nvn_format, uint32_t width, uint32_t height,
                        const void *src, size_t src_size,
                        void *dst, size_t dst_pitch,
                        nvn_bc_output *output_format);

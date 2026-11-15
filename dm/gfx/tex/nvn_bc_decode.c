#include "nvn_bc_decode.h"

#define BCDEC_BC4BC5_PRECISE
#define BCDEC_IMPLEMENTATION
#include "bcdec.h"

#include <limits.h>
#include <string.h>

typedef struct {
    unsigned block_bytes;
    unsigned pixel_bytes;
    nvn_bc_output output;
    unsigned kind;
    unsigned opaque;
} bc_format;

/* kind: 1=BC1, 2=BC2, 3=BC3, 4=BC4, 5=BC5, 6=BC6H, 7=BC7 */
static int bc_format_for_nvn(int f, bc_format *out) {
    bc_format v;
    switch (f) {
        case NVN_BC_DXT1_RGB: case NVN_BC_DXT1_RGBA:
        case NVN_BC_DXT1_RGB_SRGB: case NVN_BC_DXT1_RGBA_SRGB:
            v = (bc_format){8, 4, NVN_BC_OUTPUT_RGBA8, 1,
                            f == NVN_BC_DXT1_RGB || f == NVN_BC_DXT1_RGB_SRGB};
            break;
        case NVN_BC_DXT3: case NVN_BC_DXT3_SRGB:
            v = (bc_format){16, 4, NVN_BC_OUTPUT_RGBA8, 2, 0}; break;
        case NVN_BC_DXT5: case NVN_BC_DXT5_SRGB:
            v = (bc_format){16, 4, NVN_BC_OUTPUT_RGBA8, 3, 0}; break;
        case NVN_BC4_UNORM:
            v = (bc_format){8, 1, NVN_BC_OUTPUT_R8, 4, 0}; break;
        case NVN_BC4_SNORM:
            v = (bc_format){8, 1, NVN_BC_OUTPUT_R8_SNORM, 4, 0}; break;
        case NVN_BC5_UNORM:
            v = (bc_format){16, 2, NVN_BC_OUTPUT_RG8, 5, 0}; break;
        case NVN_BC5_SNORM:
            v = (bc_format){16, 2, NVN_BC_OUTPUT_RG8_SNORM, 5, 0}; break;
        case NVN_BC6H_SFLOAT:
        case NVN_BC6H_UFLOAT:
            v = (bc_format){16, 8, NVN_BC_OUTPUT_RGBA16_FLOAT, 6, f == NVN_BC6H_SFLOAT}; break;
        case NVN_BC7_UNORM: case NVN_BC7_SRGB:
            v = (bc_format){16, 4, NVN_BC_OUTPUT_RGBA8, 7, 0}; break;
        default: return -1;
    }
    *out = v;
    return 0;
}

static size_t bytes_per_pixel(nvn_bc_output f) {
    switch (f) {
        case NVN_BC_OUTPUT_RGBA8: return 4;
        case NVN_BC_OUTPUT_R8: return 1;
        case NVN_BC_OUTPUT_RG8: return 2;
        case NVN_BC_OUTPUT_RGBA16_FLOAT: return 8;
        case NVN_BC_OUTPUT_R8_SNORM: return 1;
        case NVN_BC_OUTPUT_RG8_SNORM: return 2;
    }
    return 0;
}

int nvn_bc_decode_level(int format, uint32_t width, uint32_t height,
                        const void *src_data, size_t src_size,
                        void *dst_data, size_t dst_pitch,
                        nvn_bc_output *output_format) {
    bc_format fmt;
    if (!width || !height || !src_data || !dst_data ||
        bc_format_for_nvn(format, &fmt)) return -1;

    size_t blocks_x = ((size_t)width + 3) / 4;
    size_t blocks_y = ((size_t)height + 3) / 4;
    if (blocks_x > SIZE_MAX / blocks_y ||
        blocks_x * blocks_y > SIZE_MAX / fmt.block_bytes) return -1;
    size_t required = blocks_x * blocks_y * fmt.block_bytes;
    size_t pixel_bytes = bytes_per_pixel(fmt.output);
    if (!pixel_bytes || src_size < required || width > SIZE_MAX / pixel_bytes ||
        dst_pitch < (size_t)width * pixel_bytes || height > SIZE_MAX / dst_pitch) return -1;

    const uint8_t *src = (const uint8_t *)src_data;
    uint8_t *dst = (uint8_t *)dst_data;
    uint8_t block_rgba[4 * 4 * 4];
    uint8_t block_r[4 * 4];
    uint8_t block_rg[4 * 4 * 2];
    uint16_t block_rgbh[4 * 4 * 3];

    for (size_t by = 0; by < blocks_y; by++) {
        for (size_t bx = 0; bx < blocks_x; bx++) {
            const uint8_t *block = src + (by * blocks_x + bx) * fmt.block_bytes;
            void *decoded = NULL;
            size_t decoded_pitch = 0;

            switch (fmt.kind) {
                case 1: bcdec_bc1(block, block_rgba, 16); decoded = block_rgba; decoded_pitch = 16; break;
                case 2: bcdec_bc2(block, block_rgba, 16); decoded = block_rgba; decoded_pitch = 16; break;
                case 3: bcdec_bc3(block, block_rgba, 16); decoded = block_rgba; decoded_pitch = 16; break;
                case 4:
                    bcdec_bc4(block, block_r, 4, format == NVN_BC4_SNORM);
                    decoded = block_r; decoded_pitch = 4;
                    break;
                case 5:
                    bcdec_bc5(block, block_rg, 8, format == NVN_BC5_SNORM);
                    decoded = block_rg; decoded_pitch = 8;
                    break;
                case 6:
                    bcdec_bc6h_half(block, block_rgbh, 3 * 4 * (int)sizeof(uint16_t), fmt.opaque);
                    decoded = block_rgbh; decoded_pitch = 3 * 4 * sizeof(uint16_t);
                    break;
                case 7: bcdec_bc7(block, block_rgba, 16); decoded = block_rgba; decoded_pitch = 16; break;
            }

            size_t x0 = bx * 4, y0 = by * 4;
            size_t copy_w = width - x0 < 4 ? width - x0 : 4;
            size_t copy_h = height - y0 < 4 ? height - y0 : 4;
            for (size_t y = 0; y < copy_h; y++) {
                uint8_t *row = dst + (y0 + y) * dst_pitch + x0 * pixel_bytes;
                const uint8_t *src_row = (const uint8_t *)decoded + y * decoded_pitch;
                if (fmt.kind == 6) {
                    const uint16_t *rgb = (const uint16_t *)src_row;
                    uint16_t *rgba = (uint16_t *)row;
                    for (size_t x = 0; x < copy_w; x++) {
                        rgba[x * 4 + 0] = rgb[x * 3 + 0];
                        rgba[x * 4 + 1] = rgb[x * 3 + 1];
                        rgba[x * 4 + 2] = rgb[x * 3 + 2];
                        rgba[x * 4 + 3] = 0x3c00; /* IEEE-754 half 1.0 */
                    }
                } else {
                    memcpy(row, src_row, copy_w * pixel_bytes);
                    if (fmt.opaque) for (size_t x = 0; x < copy_w; x++) row[x * 4 + 3] = 255;
                }
            }
        }
    }
    if (output_format) *output_format = fmt.output;
    return 0;
}

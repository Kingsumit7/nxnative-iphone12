#define BCDEC_BC4BC5_PRECISE
#define BCDEC_IMPLEMENTATION
#include "../../third_party/bcdec/bcdec.h"
#include "nvn_bc_decode.h"
#include <limits.h>
#include <stdint.h>
#include <string.h>

static int format_info(uint32_t f, size_t *block_bytes, int *kind, int *is_signed, int *opaque, int *srgb) {
    *is_signed = 0; *opaque = 0; *srgb = 0;
    switch (f) {
        case 0x41: *block_bytes = 8;  *kind = 1; *opaque = 1; return 0; /* RGB_DXT1 */
        case 0x42: *block_bytes = 8;  *kind = 1; return 0;              /* DXT1 */
        case 0x43: *block_bytes = 16; *kind = 2; return 0;              /* DXT3 */
        case 0x44: *block_bytes = 16; *kind = 3; return 0;              /* DXT5 */
        case 0x45: *block_bytes = 8;  *kind = 1; *opaque = 1; *srgb = 1; return 0;
        case 0x46: *block_bytes = 8;  *kind = 1; *srgb = 1; return 0;
        case 0x47: *block_bytes = 16; *kind = 2; *srgb = 1; return 0;
        case 0x48: *block_bytes = 16; *kind = 3; *srgb = 1; return 0;
        case 0x49: *block_bytes = 8;  *kind = 4; return 0;              /* BC4 UNORM */
        case 0x4A: *block_bytes = 8;  *kind = 4; *is_signed = 1; return 0;
        case 0x4B: *block_bytes = 16; *kind = 5; return 0;              /* BC5 UNORM */
        case 0x4C: *block_bytes = 16; *kind = 5; *is_signed = 1; return 0;
        case 0x4D: *block_bytes = 16; *kind = 7; return 0;              /* BC7 */
        case 0x4E: *block_bytes = 16; *kind = 7; *srgb = 1; return 0;
        case 0x4F: *block_bytes = 16; *kind = 6; *is_signed = 1; return 0; /* BC6H SFLOAT */
        case 0x50: *block_bytes = 16; *kind = 6; return 0;              /* BC6H UFLOAT */
        default: return -1;
    }
}

int nvn_bc_decode_image(uint32_t format, const void *src, size_t src_size,
                        uint32_t width, uint32_t height,
                        float *dst, size_t dst_stride) {
    size_t block_bytes, blocks_x, blocks_y, required, min_stride;
    int kind, is_signed, opaque, srgb;
    if (!src || !dst || !width || !height ||
        format_info(format, &block_bytes, &kind, &is_signed, &opaque, &srgb)) return -1;
    (void)srgb; /* Encoded sRGB values are deliberately retained for ASTC sRGB output. */
    blocks_x = ((size_t)width + 3u) / 4u;
    blocks_y = ((size_t)height + 3u) / 4u;
    if (blocks_x > SIZE_MAX / blocks_y || blocks_x * blocks_y > SIZE_MAX / block_bytes) return -1;
    required = blocks_x * blocks_y * block_bytes;
    if (src_size < required || width > SIZE_MAX / (4u * sizeof(float))) return -1;
    min_stride = (size_t)width * 4u * sizeof(float);
    if (dst_stride < min_stride || height > SIZE_MAX / dst_stride) return -1;

    const uint8_t *input = (const uint8_t *)src;
    for (size_t by = 0; by < blocks_y; ++by) for (size_t bx = 0; bx < blocks_x; ++bx) {
        const uint8_t *block = input + (by * blocks_x + bx) * block_bytes;
        float decoded_f[4 * 4 * 4];
        uint8_t decoded_u8[4 * 4 * 4];
        if (kind == 1 || kind == 2 || kind == 3 || kind == 7) {
            if (kind == 1) bcdec_bc1(block, decoded_u8, 16);
            else if (kind == 2) bcdec_bc2(block, decoded_u8, 16);
            else if (kind == 3) bcdec_bc3(block, decoded_u8, 16);
            else bcdec_bc7(block, decoded_u8, 16);
            for (unsigned py = 0; py < 4; ++py) for (unsigned px = 0; px < 4; ++px) {
                const uint8_t *p = decoded_u8 + (py * 4u + px) * 4u;
                float *q = decoded_f + (py * 4u + px) * 4u;
                q[0] = p[0] / 255.0f; q[1] = p[1] / 255.0f; q[2] = p[2] / 255.0f;
                q[3] = opaque ? 1.0f : p[3] / 255.0f;
            }
        } else if (kind == 4) {
            float channel[16];
            bcdec_bc4_float(block, channel, 4 * (int)sizeof(float), is_signed);
            for (unsigned i = 0; i < 16; ++i) {
                decoded_f[i * 4 + 0] = channel[i]; decoded_f[i * 4 + 1] = 0.0f;
                decoded_f[i * 4 + 2] = 0.0f; decoded_f[i * 4 + 3] = 1.0f;
            }
        } else if (kind == 5) {
            float channels[16 * 2];
            bcdec_bc5_float(block, channels, 4 * 2 * (int)sizeof(float), is_signed);
            for (unsigned i = 0; i < 16; ++i) {
                decoded_f[i * 4 + 0] = channels[i * 2 + 0]; decoded_f[i * 4 + 1] = channels[i * 2 + 1];
                decoded_f[i * 4 + 2] = 0.0f; decoded_f[i * 4 + 3] = 1.0f;
            }
        } else {
            float rgb[16 * 3];
            bcdec_bc6h_float(block, rgb, 4 * 3 * (int)sizeof(float), is_signed);
            for (unsigned i = 0; i < 16; ++i) {
                decoded_f[i * 4 + 0] = rgb[i * 3 + 0]; decoded_f[i * 4 + 1] = rgb[i * 3 + 1];
                decoded_f[i * 4 + 2] = rgb[i * 3 + 2]; decoded_f[i * 4 + 3] = 1.0f;
            }
        }
        for (unsigned py = 0; py < 4; ++py) {
            uint32_t y = (uint32_t)(by * 4u + py);
            if (y >= height) continue;
            float *row = (float *)((uint8_t *)dst + (size_t)y * dst_stride);
            for (unsigned px = 0; px < 4; ++px) {
                uint32_t x = (uint32_t)(bx * 4u + px);
                if (x >= width) continue;
                memcpy(row + (size_t)x * 4u, decoded_f + (py * 4u + px) * 4u, 4u * sizeof(float));
            }
        }
    }
    return 0;
}

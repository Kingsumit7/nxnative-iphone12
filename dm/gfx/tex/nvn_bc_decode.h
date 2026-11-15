// NVN BC texture block decoder. Output is interleaved RGBA32F.
#pragma once
#include <stddef.h>
#include <stdint.h>

// Supported NVN formats: 0x41-0x50 (BC1 through BC7, including signed BC4/5
// and signed/unsigned BC6H). Returns 0 on success, -1 on invalid input/format.
// RGB channels from sRGB BC formats remain encoded in the sRGB domain so the
// caller can select an sRGB ASTC destination format without double conversion.
// dst_row_stride_bytes must be at least width * 4 * sizeof(float).
int nvn_bc_decode_image(uint32_t nvn_format, const void *src, size_t src_size,
                        uint32_t width, uint32_t height,
                        float *dst_rgba, size_t dst_row_stride_bytes);

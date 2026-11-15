// NVN BC to ASTC 4x4 image transcoder.
#pragma once
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Profiles are the ASTC Encoder profile values. Use LDR for linear BC formats,
// LDR_SRGB for sRGB BC formats, and HDR_RGB_LDR_A for BC6H or signed BC4/BC5.
// Returns 0 on success, -1 on invalid input/profile/encoding failure, and -2
// when dst_capacity is too small. On success or -2, *dst_size receives the
// required/produced tightly packed ASTC 4x4 byte count.
int nvn_bc_to_astc_image(uint32_t nvn_format, const void *src, size_t src_size,
                         uint32_t width, uint32_t height,
                         uint32_t astc_profile, float quality,
                         void *dst, size_t dst_capacity, size_t *dst_size);

#ifdef __cplusplus
}
#endif

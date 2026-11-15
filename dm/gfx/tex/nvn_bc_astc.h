// NVN BC to ASTC 4x4 image transcoder.
#pragma once
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum nvn_astc_profile {
    NVN_ASTC_PROFILE_LDR = 0,
    NVN_ASTC_PROFILE_LDR_SRGB = 1,
    NVN_ASTC_PROFILE_HDR_RGB_LDR_A = 2,
    NVN_ASTC_PROFILE_HDR = 3
} nvn_astc_profile;

// Transcodes one tightly packed BC image to ASTC 4x4 blocks.
// dst_size receives the required output size when dimensions and format are valid.
// Returns 0 on success, -1 for invalid input/profile, -2 for insufficient output,
// or -3 when the decoder/encoder reports an error.
// BC6H requires an HDR profile. sRGB BC formats require LDR_SRGB.
int nvn_bc_to_astc_image(uint32_t nvn_format, const void *src, size_t src_size,
                         uint32_t width, uint32_t height,
                         nvn_astc_profile profile, float quality,
                         uint8_t *dst, size_t dst_capacity, size_t *dst_size);

#ifdef __cplusplus
}
#endif

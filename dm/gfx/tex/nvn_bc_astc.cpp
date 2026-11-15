#include "nvn_bc_astc.h"
#include "nvn_bc_decode.h"
#include "astcenc.h"

#include <cmath>
#include <limits>
#include <new>
#include <vector>

namespace {
bool is_srgb(uint32_t f) { return (f >= 0x45 && f <= 0x48) || f == 0x4E; }
bool is_bc6h(uint32_t f) { return f == 0x4F || f == 0x50; }
bool valid_profile(uint32_t format, nvn_astc_profile profile) {
    if (profile < NVN_ASTC_PROFILE_LDR || profile > NVN_ASTC_PROFILE_HDR) return false;
    if (is_srgb(format)) return profile == NVN_ASTC_PROFILE_LDR_SRGB;
    if (is_bc6h(format)) return profile == NVN_ASTC_PROFILE_HDR_RGB_LDR_A || profile == NVN_ASTC_PROFILE_HDR;
    if (profile == NVN_ASTC_PROFILE_LDR_SRGB) return false;
    return true;
}
astcenc_profile to_astc_profile(nvn_astc_profile profile) {
    switch (profile) {
        case NVN_ASTC_PROFILE_LDR: return ASTCENC_PRF_LDR;
        case NVN_ASTC_PROFILE_LDR_SRGB: return ASTCENC_PRF_LDR_SRGB;
        case NVN_ASTC_PROFILE_HDR_RGB_LDR_A: return ASTCENC_PRF_HDR_RGB_LDR_A;
        case NVN_ASTC_PROFILE_HDR: return ASTCENC_PRF_HDR;
    }
    return ASTCENC_PRF_LDR;
}
}

extern "C" int nvn_bc_to_astc_image(uint32_t format, const void *src, size_t src_size,
                                    uint32_t width, uint32_t height,
                                    nvn_astc_profile profile, float quality,
                                    uint8_t *dst, size_t dst_capacity, size_t *dst_size) {
    if (dst_size) *dst_size = 0;
    if (!src || !dst || !dst_size || !width || !height ||
        !std::isfinite(quality) || quality < ASTCENC_PRE_FASTEST ||
        quality > ASTCENC_PRE_EXHAUSTIVE || !valid_profile(format, profile)) return -1;

    const size_t blocks_x = (static_cast<size_t>(width) + 3u) / 4u;
    const size_t blocks_y = (static_cast<size_t>(height) + 3u) / 4u;
    if (blocks_x > std::numeric_limits<size_t>::max() / blocks_y ||
        blocks_x * blocks_y > std::numeric_limits<size_t>::max() / 16u) return -1;
    const size_t required = blocks_x * blocks_y * 16u;
    *dst_size = required;
    if (dst_capacity < required) return -2;

    if (static_cast<size_t>(width) > std::numeric_limits<size_t>::max() / height / 4u / sizeof(float)) return -1;
    try {
        std::vector<float> pixels(static_cast<size_t>(width) * height * 4u);
        if (nvn_bc_decode_image(format, src, src_size, width, height,
                                pixels.data(), static_cast<size_t>(width) * 4u * sizeof(float)) != 0) return -1;

        astcenc_config config{};
        astcenc_error status = astcenc_config_init(to_astc_profile(profile), 4, 4, 1,
                                                    quality, 0, &config);
        if (status != ASTCENC_SUCCESS) return -3;
        astcenc_context *context = nullptr;
        status = astcenc_context_alloc(&config, 1, &context, nullptr);
        if (status != ASTCENC_SUCCESS) return -3;

        void *slices[] = {pixels.data()};
        astcenc_image image{width, height, 1, ASTCENC_TYPE_F32, slices};
        const astcenc_swizzle swizzle{ASTCENC_SWZ_R, ASTCENC_SWZ_G,
                                      ASTCENC_SWZ_B, ASTCENC_SWZ_A};
        status = astcenc_compress_image(context, &image, &swizzle, dst, required, 0);
        astcenc_context_free(context);
        return status == ASTCENC_SUCCESS ? 0 : -3;
    } catch (const std::bad_alloc &) {
        return -3;
    } catch (...) {
        return -3;
    }
}

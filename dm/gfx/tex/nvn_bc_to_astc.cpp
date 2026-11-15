#include "nvn_bc_to_astc.h"
#include "nvn_bc_decode.h"
#include "astcenc.h"

#include <cmath>
#include <cstdint>
#include <limits>
#include <vector>

namespace {
bool profile_matches(uint32_t format, uint32_t profile) {
    const bool srgb = format >= 0x45 && format <= 0x48 || format == 0x4E;
    const bool hdr = format == 0x4F || format == 0x50 ||
                     format == 0x4A || format == 0x4C;
    if (srgb) return profile == ASTCENC_PRF_LDR_SRGB;
    if (hdr) return profile == ASTCENC_PRF_HDR_RGB_LDR_A ||
                     profile == ASTCENC_PRF_HDR;
    return profile == ASTCENC_PRF_LDR ||
           profile == ASTCENC_PRF_HDR_RGB_LDR_A ||
           profile == ASTCENC_PRF_HDR;
}
}

extern "C" int nvn_bc_to_astc_image(uint32_t format, const void *src,
                                     size_t src_size, uint32_t width,
                                     uint32_t height, uint32_t profile,
                                     float quality, void *dst,
                                     size_t dst_capacity, size_t *dst_size) {
    if (dst_size) *dst_size = 0;
    if (!src || !dst_size || !width || !height ||
        !std::isfinite(quality) || quality < 0.0f || quality > 100.0f ||
        !profile_matches(format, profile)) return -1;

    const size_t blocks_x = (static_cast<size_t>(width) + 3u) / 4u;
    const size_t blocks_y = (static_cast<size_t>(height) + 3u) / 4u;
    if (blocks_x > std::numeric_limits<size_t>::max() / blocks_y ||
        blocks_x * blocks_y > std::numeric_limits<size_t>::max() / 16u)
        return -1;
    const size_t required = blocks_x * blocks_y * 16u;
    *dst_size = required;
    if (!dst || dst_capacity < required) return -2;

    if (width > std::numeric_limits<size_t>::max() / (4u * sizeof(float)))
        return -1;
    const size_t row_stride = static_cast<size_t>(width) * 4u * sizeof(float);
    if (height > std::numeric_limits<size_t>::max() / row_stride) return -1;
    std::vector<float> pixels(static_cast<size_t>(width) * height * 4u);
    if (nvn_bc_decode_image(format, src, src_size, width, height,
                            pixels.data(), row_stride) != 0) return -1;

    astcenc_config config{};
    astcenc_error status = astcenc_config_init(
        static_cast<astcenc_profile>(profile), 4, 4, 1, quality, 0, &config);
    if (status != ASTCENC_SUCCESS) return -1;

    astcenc_context *context = nullptr;
    status = astcenc_context_alloc(&config, 1, &context, nullptr);
    if (status != ASTCENC_SUCCESS || !context) return -1;

    void *slices[] = { pixels.data() };
    astcenc_image image{width, height, 1, ASTCENC_TYPE_F32, slices};
    const astcenc_swizzle swizzle{
        ASTCENC_SWZ_R, ASTCENC_SWZ_G, ASTCENC_SWZ_B, ASTCENC_SWZ_A
    };
    status = astcenc_compress_image(
        context, &image, &swizzle, static_cast<uint8_t *>(dst),
        dst_capacity, 0);
    astcenc_context_free(context);
    if (status != ASTCENC_SUCCESS) {
        *dst_size = 0;
        return -1;
    }
    return 0;
}

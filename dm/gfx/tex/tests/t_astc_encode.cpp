#include "astcenc.h"
#include <array>
#include <cstdio>
#include <cstdint>
#include <vector>

static bool encode_profile(astcenc_profile profile, float hdr_scale) {
    constexpr unsigned int width = 5;
    constexpr unsigned int height = 3;
    constexpr size_t expected_bytes = 2 * 1 * 16;
    std::array<float, width * height * 4> pixels{};
    for (unsigned int y = 0; y < height; ++y) {
        for (unsigned int x = 0; x < width; ++x) {
            const size_t i = (y * width + x) * 4;
            pixels[i + 0] = (0.1f + 0.1f * x) * hdr_scale;
            pixels[i + 1] = (0.2f + 0.1f * y) * hdr_scale;
            pixels[i + 2] = 0.35f * hdr_scale;
            pixels[i + 3] = 1.0f;
        }
    }
    void* slices[] = { pixels.data() };
    astcenc_image image{width, height, 1, ASTCENC_TYPE_F32, slices};
    const astcenc_swizzle swizzle{ASTCENC_SWZ_R, ASTCENC_SWZ_G, ASTCENC_SWZ_B, ASTCENC_SWZ_A};

    astcenc_config config{};
    astcenc_error status = astcenc_config_init(profile, 4, 4, 1, ASTCENC_PRE_FAST, 0, &config);
    if (status != ASTCENC_SUCCESS) {
        std::fprintf(stderr, "config_init failed: %s\n", astcenc_get_error_string(status));
        return false;
    }

    astcenc_context* context = nullptr;
    status = astcenc_context_alloc(&config, 1, &context, nullptr);
    if (status != ASTCENC_SUCCESS) {
        std::fprintf(stderr, "context_alloc failed: %s\n", astcenc_get_error_string(status));
        return false;
    }

    std::vector<uint8_t> output(expected_bytes, 0);
    status = astcenc_compress_image(context, &image, &swizzle, output.data(), output.size(), 0);
    astcenc_context_free(context);
    if (status != ASTCENC_SUCCESS) {
        std::fprintf(stderr, "compress_image failed: %s\n", astcenc_get_error_string(status));
        return false;
    }
    bool any_nonzero = false;
    for (uint8_t byte : output) any_nonzero |= byte != 0;
    return any_nonzero;
}

int main() {
    if (!encode_profile(ASTCENC_PRF_LDR, 1.0f)) return 1;
    if (!encode_profile(ASTCENC_PRF_LDR_SRGB, 1.0f)) return 2;
    if (!encode_profile(ASTCENC_PRF_HDR, 4.0f)) return 3;
    std::puts("PASS: ASTC LDR, sRGB, and HDR encode smoke tests");
    return 0;
}

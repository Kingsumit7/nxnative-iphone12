#include "nvn_bc_to_astc.h"
#include "astcenc.h"

#include <cstdint>
#include <cstdio>
#include <vector>

static bool run_case(uint32_t format, uint32_t profile) {
    constexpr uint32_t width = 5, height = 3;
    const size_t input_block_bytes =
        (format == 0x41 || format == 0x42 || format == 0x45 ||
         format == 0x46 || format == 0x49 || format == 0x4A) ? 8u : 16u;
    const size_t input_size = 2u * input_block_bytes;
    std::vector<uint8_t> input(input_size, 0);
    std::vector<uint8_t> output(32, 0);
    size_t output_size = 0;
    int rc = nvn_bc_to_astc_image(format, input.data(), input.size(),
                                  width, height, profile, ASTCENC_PRE_FAST,
                                  output.data(), output.size(), &output_size);
    if (rc != 0 || output_size != 32) {
        std::fprintf(stderr, "format 0x%02X failed, rc=%d size=%zu\n",
                     format, rc, output_size);
        return false;
    }
    bool nonzero = false;
    for (uint8_t byte : output) nonzero |= byte != 0;
    return nonzero;
}

int main() {
    for (uint32_t f = 0x41; f <= 0x50; ++f) {
        uint32_t profile = ASTCENC_PRF_LDR;
        if (f >= 0x45 && f <= 0x48 || f == 0x4E)
            profile = ASTCENC_PRF_LDR_SRGB;
        if (f == 0x4A || f == 0x4C || f == 0x4F || f == 0x50)
            profile = ASTCENC_PRF_HDR_RGB_LDR_A;
        if (!run_case(f, profile)) return 1;
    }

    uint8_t input[32] = {};
    uint8_t output[32] = {};
    size_t output_size = 0;
    if (nvn_bc_to_astc_image(0x45, input, 8, 5, 3, ASTCENC_PRF_LDR,
                             ASTCENC_PRE_FAST, output, sizeof(output),
                             &output_size) != -1) return 2;
    if (nvn_bc_to_astc_image(0x41, input, 8, 5, 3, ASTCENC_PRF_LDR,
                             ASTCENC_PRE_FAST, output, 8,
                             &output_size) != -2 || output_size != 32) return 3;
    if (nvn_bc_to_astc_image(0x41, input, 7, 5, 3, ASTCENC_PRF_LDR,
                             ASTCENC_PRE_FAST, output, sizeof(output),
                             &output_size) != -1) return 4;
    std::puts("PASS: BC1-BC7 to ASTC 4x4, 16 format IDs, profile and bounds checks");
    return 0;
}

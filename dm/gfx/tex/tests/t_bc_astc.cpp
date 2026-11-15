#include "nvn_bc_astc.h"
#include <array>
#include <cstdint>
#include <cstdio>
#include <vector>

static unsigned block_bytes(uint32_t f) {
    return (f == 0x41 || f == 0x42 || f == 0x45 || f == 0x46 ||
            f == 0x49 || f == 0x4A) ? 8u : 16u;
}
static nvn_astc_profile profile_for(uint32_t f) {
    if ((f >= 0x45 && f <= 0x48) || f == 0x4E) return NVN_ASTC_PROFILE_LDR_SRGB;
    if (f == 0x4F || f == 0x50 || f == 0x4A || f == 0x4C)
        return NVN_ASTC_PROFILE_HDR_RGB_LDR_A;
    return NVN_ASTC_PROFILE_LDR;
}

int main() {
    unsigned checks = 0;
    constexpr uint32_t width = 5, height = 3;
    for (uint32_t format = 0x41; format <= 0x50; ++format) {
        std::vector<uint8_t> bc(2u * block_bytes(format), 0);
        std::array<uint8_t, 32> astc{};
        size_t written = 0;
        int rc = nvn_bc_to_astc_image(format, bc.data(), bc.size(), width, height,
                                      profile_for(format), 10.0f,
                                      astc.data(), astc.size(), &written);
        if (rc != 0 || written != astc.size()) {
            std::fprintf(stderr, "format 0x%02X failed: rc=%d size=%zu\n", format, rc, written);
            return 1;
        }
        bool nonzero = false;
        for (uint8_t v : astc) nonzero |= v != 0;
        if (!nonzero) return 2;
        ++checks;
    }

    std::array<uint8_t, 16> bc{};
    std::array<uint8_t, 32> astc{};
    size_t written = 0;
    if (nvn_bc_to_astc_image(0x45, bc.data(), bc.size(), 4, 4,
                             NVN_ASTC_PROFILE_LDR, 10.0f,
                             astc.data(), astc.size(), &written) != -1) return 3;
    ++checks;
    if (nvn_bc_to_astc_image(0x4F, bc.data(), bc.size(), 4, 4,
                             NVN_ASTC_PROFILE_LDR, 10.0f,
                             astc.data(), astc.size(), &written) != -1) return 4;
    ++checks;
    if (nvn_bc_to_astc_image(0x42, bc.data(), bc.size(), 4, 4,
                             NVN_ASTC_PROFILE_LDR, 10.0f,
                             astc.data(), 15, &written) != -2 || written != 16) return 5;
    ++checks;
    if (nvn_bc_to_astc_image(0x42, bc.data(), 7, 4, 4,
                             NVN_ASTC_PROFILE_LDR, 10.0f,
                             astc.data(), astc.size(), &written) != -1) return 6;
    ++checks;
    if (nvn_bc_to_astc_image(0x42, bc.data(), bc.size(), 4, 4,
                             NVN_ASTC_PROFILE_LDR, 101.0f,
                             astc.data(), astc.size(), &written) != -1) return 7;
    ++checks;

    std::printf("PASS: %u BC-to-ASTC checks across NVN formats 0x41-0x50\n", checks);
    return 0;
}

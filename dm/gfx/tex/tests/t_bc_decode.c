#include "../nvn_bc_decode.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void test_bc1_opaque_and_crop(void) {
    /* One BC1 block with both endpoints black. */
    uint8_t src[8] = {0};
    uint8_t dst[3 * 2 * 4];
    memset(dst, 0xCD, sizeof dst);
    nvn_bc_output out = NVN_BC_OUTPUT_R8;
    assert(nvn_bc_decode_level(NVN_BC_DXT1_RGB, 3, 2, src, sizeof src,
                               dst, 3 * 4, &out) == 0);
    assert(out == NVN_BC_OUTPUT_RGBA8);
    for (int i = 0; i < 6; i++) assert(dst[i * 4 + 3] == 255);
}

static void test_bc2_and_bc3(void) {
    uint8_t src[16] = {0}, dst[4 * 4 * 4] = {0};
    nvn_bc_output out;
    assert(nvn_bc_decode_level(NVN_BC_DXT3, 4, 4, src, sizeof src,
                               dst, 4 * 4, &out) == 0);
    assert(out == NVN_BC_OUTPUT_RGBA8);
    assert(nvn_bc_decode_level(NVN_BC_DXT5, 4, 4, src, sizeof src,
                               dst, 4 * 4, &out) == 0);
    assert(out == NVN_BC_OUTPUT_RGBA8);
}

static void test_rejects_short_source_and_pitch(void) {
    uint8_t src[8] = {0}, dst[64] = {0};
    assert(nvn_bc_decode_level(NVN_BC_DXT1_RGBA, 4, 4, src, 7, dst, 16, NULL) == -1);
    assert(nvn_bc_decode_level(NVN_BC_DXT1_RGBA, 4, 4, src, 8, dst, 15, NULL) == -1);
}

static void test_bc4_and_bc5_outputs(void) {
    uint8_t src[16] = {0}, dst[4 * 4 * 8] = {0};
    nvn_bc_output out;
    assert(nvn_bc_decode_level(NVN_BC4_UNORM, 4, 4, src, 8, dst, 4, &out) == 0);
    assert(out == NVN_BC_OUTPUT_R8);
    assert(nvn_bc_decode_level(NVN_BC5_UNORM, 4, 4, src, 16, dst, 8, &out) == 0);
    assert(out == NVN_BC_OUTPUT_RG8);
    assert(nvn_bc_decode_level(NVN_BC4_SNORM, 4, 4, src, 8, dst, 4, &out) == 0);
    assert(out == NVN_BC_OUTPUT_R8_SNORM);
    assert(nvn_bc_decode_level(NVN_BC5_SNORM, 4, 4, src, 16, dst, 8, &out) == 0);
    assert(out == NVN_BC_OUTPUT_RG8_SNORM);
}

static void test_bc6h_and_bc7(void) {
    uint8_t src[16] = {0}, dst[4 * 4 * 8] = {0};
    nvn_bc_output out;
    assert(nvn_bc_decode_level(NVN_BC6H_UFLOAT, 4, 4, src, 16, dst, 32, &out) == 0);
    assert(out == NVN_BC_OUTPUT_RGBA16_FLOAT);
    assert(((uint16_t *)dst)[3] == 0x3c00);
    assert(nvn_bc_decode_level(NVN_BC7_UNORM, 4, 4, src, 16, dst, 16, &out) == 0);
    assert(out == NVN_BC_OUTPUT_RGBA8);
}

int main(void) {
    test_bc1_opaque_and_crop();
    test_bc2_and_bc3();
    test_rejects_short_source_and_pitch();
    test_bc4_and_bc5_outputs();
    test_bc6h_and_bc7();
    puts("BC decoder tests passed");
    return 0;
}

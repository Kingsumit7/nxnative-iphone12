#include "../nvn_bc_decode.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int checks, failures;
#define CHECK(c, msg) do { ++checks; if (!(c)) { ++failures; printf("FAIL: %s\n", msg); } } while (0)

int main(void) {
    static const uint32_t formats[] = {0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4A,0x4B,0x4C,0x4D,0x4E,0x4F,0x50};
    uint8_t compressed[64] = {0};
    float pixels[5 * 3 * 4];
    for (unsigned i = 0; i < sizeof formats / sizeof formats[0]; ++i) {
        memset(pixels, 0xCD, sizeof pixels);
        int rc = nvn_bc_decode_image(formats[i], compressed, sizeof compressed, 5, 3, pixels, 5 * 4 * sizeof(float));
        CHECK(rc == 0, "all NVN BC format IDs decode a 5x3 edge-sized image");
        if (!rc) {
            for (unsigned p = 0; p < 15 * 4; ++p) CHECK(isfinite(pixels[p]), "decoded pixel channels are finite for zero blocks");
        }
    }
    CHECK(nvn_bc_decode_image(0x40, compressed, sizeof compressed, 4, 4, pixels, 4 * 4 * sizeof(float)) == -1, "unknown format rejected");
    CHECK(nvn_bc_decode_image(0x42, compressed, 7, 4, 4, pixels, 4 * 4 * sizeof(float)) == -1, "truncated block rejected");
    CHECK(nvn_bc_decode_image(0x42, compressed, sizeof compressed, 4, 4, pixels, 4 * sizeof(float)) == -1, "short destination stride rejected");
    CHECK(nvn_bc_decode_image(0x42, compressed, sizeof compressed, 0, 4, pixels, 4 * 4 * sizeof(float)) == -1, "zero width rejected");
    printf("%s: %d/%d checks\n", failures ? "FAILED" : "PASS", checks - failures, checks);
    return failures ? 1 : 0;
}

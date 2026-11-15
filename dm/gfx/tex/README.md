# Texture formats and layout conversion

The texture subsystem is called by the NVN Metal replay layer, `../nvn/nvn_mtl.m`. It currently provides:

1. NVN texture format to Metal pixel-format mapping.
2. CPU deswizzling from the Switch's block-linear layout to row-major texture data.
3. A Metal compute implementation of the same deswizzling operation.

A CPU BC1–BC7 block decoder is now vendored and exposed through `nvn_bc_decode_image()`. It decodes the NVN BC format IDs 0x41–0x50 into RGBA32F pixels, including signed BC4/BC5 and signed/unsigned BC6H. The decoder has initial format and bounds tests. Arm ASTC Encoder 5.7.1 is now vendored under `dm/third_party/astc-encoder`. The standalone `nvn_bc_to_astc_image()` wrapper now connects the BC decoder to ASTC 4x4 encoding for LDR, sRGB, and HDR profiles. Its independent test covers all NVN BC format IDs 0x41–0x50, output sizing, profile mismatches, truncated input, and insufficient output capacity. The wrapper is not yet connected to the texture upload path. The iOS source generator now includes C++ files and emits the ASTC encoder's A14 NEON compile flags. Use the LDR profile for linear BC textures, LDR_SRGB for sRGB BC textures, and HDR_RGB_LDR_A or HDR for BC6H. Signed BC4/BC5 data also needs an HDR profile if its negative channel values must be preserved. The wrapper emits tightly packed ASTC 4x4 blocks and reports the required output size before returning an insufficient-capacity error. The current renderer still creates BC textures directly and refuses devices without BC texture support. This excludes iPhone 12 and other A14 devices.

## Files

| File | Contents |
|---|---|
| `nvn_bc_decode.h`, `nvn_bc_decode.c` | CPU block decoder for NVN BC1–BC7 formats, using the vendored `dm/third_party/bcdec` implementation. Outputs RGBA32F pixels and preserves signed/HDR channel values. |\n| `nvn_bc_to_astc.h`, `nvn_bc_to_astc.cpp` | Standalone BC-to-ASTC 4x4 transcoder. Decodes BC data to RGBA32F, then encodes with the selected ASTC LDR, sRGB, or HDR profile. |
| `nvn_fmt_mtl.h`, `nvn_fmt_mtl.c` | `nvn_fmt_to_mtl()`: NVN format (0x01–0x9C) to Metal pixel format and component remapping. Returns 1 for an exact match, 2 for a substitute, and 0 when Metal has no direct counterpart. |
| `nvn_deswizzle.h`, `nvn_deswizzle.c` | `nvn_deswizzle()`: converts one level and layer from block-linear to row-major layout. `nvn_swizzle()` provides the reverse operation for tests. |
| `nvn_deswizzle_gpu.h`, `nvn_deswizzle_gpu.m` | Metal compute implementation used by the runtime to deswizzle packaged textures. The CPU path remains the fallback. |
| `tests/` | Format mapping, texture-layout, BC decoder, ASTC encoder, and BC-to-ASTC end-to-end tests. |

The Switch's Tegra X1 stores textures in 64-byte by 8-row GOBs. Levels, layers, block height, and block depth follow the layout rules in `../nvn/nvn_tex.c`. Packaged layout parameters override the automatic dimensions. See [Packaged texture block dimensions and linear storage rows](../../../docs/reference/nvn-objects.md#packaged-texture-block-dimensions-and-linear-storage-rows).

## A14 texture compatibility: BC to ASTC

Apple A14 GPUs support ASTC texture formats but do not expose the BC texture formats required by the current renderer. The existing code maps NVN BC1–BC7 and BC6H formats directly to Metal BC formats. It also checks `MTLDevice.supportsBCTextureCompression` and stops when the device lacks BC support.

The A14 implementation must add a real transcode path. It must not reinterpret BC bytes as ASTC bytes or merely change the Metal pixel-format enum.

The required pipeline is:

1. Read the original Switch texture data and determine its NVN format, dimensions, mip levels, array layers, and color-space interpretation.
2. Deswizzle the BC source into the expected linear block order.
3. Decode the BC formats, including BC1–BC5, BC6H, and BC7, while preserving signedness, alpha, and sRGB behavior. The first CPU decoder implementation now exists in `nvn_bc_decode.c`; it is not yet connected to texture uploads.
4. Encode the decoded pixels into an ASTC format supported by A14. Arm ASTC Encoder 5.7.1 is vendored, and its LDR, sRGB, and HDR profiles have an independent smoke test. The standalone BC-to-ASTC wrapper is implemented and tested. The Metal upload path still needs to select the correct ASTC pixel format and call the wrapper.
5. Upload the ASTC blocks with correct row pitches, mip offsets, array slices, and texture-view semantics.
6. Handle partial re-uploads and sparse texture mappings. The converted representation must remain coherent when the game updates only selected pages or tiles.
7. Add format-level tests and compare rendered output on both a BC-capable device and an A14 device.

The runtime currently depends on BC storage in several places, including texture creation, uploads, sparse texture mappings, texture views, and BC render-target shadow handling. Those paths must be audited before removing the BC capability check.

BC block decoding, the standalone BC-to-ASTC wrapper, and the vendored ASTC encoder are implemented components with CPU tests. The encoder smoke test does not yet prove image-quality correctness or runtime performance. Runtime BC-to-ASTC conversion, texture upload integration, partial/sparse update handling, and A14 rendering validation remain outstanding. Until the complete path passes tests, the runtime must continue to reject unsupported BC hardware rather than produce corrupted textures.

## Metal execution requirements

The NVN replay layer currently requires Metal 4 command queues, residency sets, command allocators, argument tables, and MTL4 encoders. The existing classic Metal path is only used for limited non-NVN work such as shader capture. It is not a complete A14 NVN rendering path.

A14 support therefore also requires a complete compatible execution path for command submission, resource residency, synchronization, upload, rendering, and presentation. Lowering the iOS deployment target alone does not provide this functionality.

## Tests

Run the CPU texture and format tests with:

```sh
make -C dm/gfx/tex/tests run
```

- `t_fmt_mtl` checks the NVN-to-Metal format table and creates supported mapped formats on the local Metal device.
- `t_tex` checks block-linear layout, deswizzling, round trips, and calculated offsets across 2D, array, and 3D textures.
- `make -C dm/gfx/tex/tests run-bc` checks all NVN BC format IDs, edge-sized images, and invalid input handling. `make -C dm/gfx/tex/tests asan-bc` runs the decoder tests under AddressSanitizer and UndefinedBehaviorSanitizer.

The ASTC encoder smoke test is run by `.github/workflows/astc-encoder-smoke.yml`. The end-to-end BC-to-ASTC test is run by `.github/workflows/bc-to-astc.yml`. It exercises all 16 NVN BC format IDs, ASTC output sizing, profile validation, truncated input, and insufficient output capacity. These tests validate the CPU transcoding path, not visual quality or A14 device rendering.

# Texture formats and layout conversion

The texture subsystem is called by the NVN Metal replay layer, `../nvn/nvn_mtl.m`. It currently provides:

1. NVN texture format to Metal pixel-format mapping.
2. CPU deswizzling from the Switch's block-linear layout to row-major texture data.
3. A Metal compute implementation of the same deswizzling operation.

A CPU BC1–BC7 block decoder is now vendored and exposed through `nvn_bc_decode_image()`. It decodes the NVN BC format IDs 0x41–0x50 into RGBA32F pixels, including signed BC4/BC5 and signed/unsigned BC6H. The decoder has initial format and bounds tests. BC-to-ASTC transcoding and runtime integration are not implemented yet. The current renderer still creates BC textures directly and refuses devices without BC texture support. This excludes iPhone 12 and other A14 devices.

## Files

| File | Contents |
|---|---|
| `nvn_bc_decode.h`, `nvn_bc_decode.c` | CPU block decoder for NVN BC1–BC7 formats, using the vendored `dm/third_party/bcdec` implementation. Outputs RGBA32F pixels and preserves signed/HDR channel values. |
| `nvn_fmt_mtl.h`, `nvn_fmt_mtl.c` | `nvn_fmt_to_mtl()`: NVN format (0x01–0x9C) to Metal pixel format and component remapping. Returns 1 for an exact match, 2 for a substitute, and 0 when Metal has no direct counterpart. |
| `nvn_deswizzle.h`, `nvn_deswizzle.c` | `nvn_deswizzle()`: converts one level and layer from block-linear to row-major layout. `nvn_swizzle()` provides the reverse operation for tests. |
| `nvn_deswizzle_gpu.h`, `nvn_deswizzle_gpu.m` | Metal compute implementation used by the runtime to deswizzle packaged textures. The CPU path remains the fallback. |
| `tests/` | Format mapping and texture-layout unit tests. |

The Switch's Tegra X1 stores textures in 64-byte by 8-row GOBs. Levels, layers, block height, and block depth follow the layout rules in `../nvn/nvn_tex.c`. Packaged layout parameters override the automatic dimensions. See [Packaged texture block dimensions and linear storage rows](../../../docs/reference/nvn-objects.md#packaged-texture-block-dimensions-and-linear-storage-rows).

## A14 texture compatibility: BC to ASTC

Apple A14 GPUs support ASTC texture formats but do not expose the BC texture formats required by the current renderer. The existing code maps NVN BC1–BC7 and BC6H formats directly to Metal BC formats. It also checks `MTLDevice.supportsBCTextureCompression` and stops when the device lacks BC support.

The A14 implementation must add a real transcode path. It must not reinterpret BC bytes as ASTC bytes or merely change the Metal pixel-format enum.

The required pipeline is:

1. Read the original Switch texture data and determine its NVN format, dimensions, mip levels, array layers, and color-space interpretation.
2. Deswizzle the BC source into the expected linear block order.
3. Decode the BC formats, including BC1–BC5, BC6H, and BC7, while preserving signedness, alpha, and sRGB behavior. The first CPU decoder implementation now exists in `nvn_bc_decode.c`; it is not yet connected to texture uploads.
4. Encode the decoded pixels into an ASTC format supported by A14. Select block dimensions based on the source and quality/performance requirements.
5. Upload the ASTC blocks with correct row pitches, mip offsets, array slices, and texture-view semantics.
6. Handle partial re-uploads and sparse texture mappings. The converted representation must remain coherent when the game updates only selected pages or tiles.
7. Add format-level tests and compare rendered output on both a BC-capable device and an A14 device.

The runtime currently depends on BC storage in several places, including texture creation, uploads, sparse texture mappings, texture views, and BC render-target shadow handling. Those paths must be audited before removing the BC capability check.

BC block decoding is an initial implemented component, not a complete A14 texture path. ASTC encoding, texture upload integration, partial/sparse update handling, and A14 rendering validation remain outstanding. Until the complete path passes tests, the runtime must continue to reject unsupported BC hardware rather than produce corrupted textures.

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

These tests validate format mappings and texture layouts. They do not yet validate BC-to-ASTC transcoding or A14 device rendering.

# NativeXRenderer in Guitar Hero: Van Halen

The game's side of NativeXRenderer (`native_x_renderer/`, the library that hooks a game's Direct3D calls and draws
on the host). The hooks and the profile are in
[`src/native_x_renderer_ghvanhalen.cpp`](../src/native_x_renderer_ghvanhalen.cpp); the library's checklist is
`native_x_renderer/docs/integrating-a-game.md`.

**Status (2026-10-26):** GH Metallica's side, translated. Both games link the same XDK Direct3D library (2.0.7978)
and the same engine, so GH Metallica's map (GHMetalRecomp's `docs/native-x-renderer-ghmetallica.md`: the device
layout, Dante's Inferno's up to the register shadow; the current objects POTCAWE's + 144; the resource objects
POTCAWE's) holds here; only the addresses differ. The default renderer from the first release on (0.1.0,
2026-10-29: `kNativeXRendererDefault` in `launcher/game.h`); `--native_x_renderer=false` or the launcher's Xenos
card draws with the Xenos path.

## The addresses

- Each of GH Metallica's 36 hooked functions was found here by a function matcher over both executables' instruction
  indexes (`ppc_index.py`): exact signatures with branch targets and the halves of `lis` addresses masked, then the
  neighborhood of matched functions, then a similarity search for the rest. Every hook lands on the same instruction.
- The library is at `0x821D0D38`–`0x821EF020` (Metallica `0x821BF408`–`0x821DD6F0`), the draws inside it
  (`0x821D28C8`–`0x821D3BA8`), the XGraphics-style helpers below it (`0x821CF210`–`0x821D0D30`).
- The device pointer is at `0x82AC13F4` (Metallica `0x82AA1574`): the renderer's block moved by `+0x1FE80` with the
  rest of `.data`.
- `scene_resolve_caller = 0x82531594` (Metallica `0x8251F04C`): the return address of the resolve of the 1280x720
  scene that the post-processing reads once a frame; NativeXRenderer applies its anti-aliasing to the scene there,
  so the HUD stays sharp.
- As in GH Metallica, DrawIndexedVerticesUP (`sub_821D32D0`) has no call id of its own: the profile names the inline
  draw end (`inline_draw_end = 13444`) and EndVertices is not hooked.

## Checked

- 2026-10-26, `--native_x_renderer=true`, 3440x1440, 120 FPS, ultrawide: the menus with their side bars, the career's
  first song with the stage widened and the highway and HUD at 16:9, the pause menu; 437 draws a frame, none failed;
  8.33 ms frames.
- recomp-tools' `Test-Games.ps1` runs the game to its first screen with both renderers, with NativeXRenderer's
  observer.

## The instruments' textures (1_5_5_5)

**Symptom (2026-10-28, Gustavo):** in the career's intro, Eddie Van Halen's guitar was white (lavender under the stage
lights), with only its relief; on the Xenos path it has its striped finish. **Cause:** the instruments' color textures
are 16-bit 1_5_5_5, a format NativeXRenderer had no host format for: their bindings sampled its white placeholder.
**Fix:** NativeXRenderer D105 loads them as the SDK's backends do (B5G5R5A1, with DiligentCore patch 0008 mapping it
to Vulkan's A1R5G5B5). Checked: the intro with 0 placeholder bindings and the guitar as on the Xenos path. **Off:**
none; the Xenos path (`native_x_renderer = false`) never had the problem.

## The bloom's lattice (NativeXRenderer D127)

**Symptom:** above 720 lines (most visible at 1440p and ultrawide), the glow around the stage's bright lights doubled
into offset copies or a grid of dots, and the menus' blurred backgrounds with the same pattern; found in GH Metallica
(a player's issue there, its `docs/bloom-lattice.md`). **Cause:** the post-processing is GH Metallica's: the 640x360
4x MSAA downsample of the scene, then 320x180 targets (EDRAM 720, 768, 816, depth 1744) for the glow pass, the bright
pass and the bloom's and the depth of field's 16-tap blurs, whose tap offsets are constants computed for 320x180
texels. Drawn at the render scale (860x360 at 3440x1440), the taps skipped texels. **Fix:** the profile sets
`unscaled_screen_fractions` (`src/native_x_renderer_ghvanhalen.cpp`): NativeXRenderer keeps the reduced copies of the
screen (640x360, 320x180) and their resolves at 1x, as on the console, the 640x360 copy averaging every pixel of the
scene under it; the rest stays at the screen's resolution. Checked (2026-11-07): a run at 3440x1440 to the intro
(`ghvanhalen_072`), the 320x180 targets and their resolves at 320x180 on the host, no warning. **Off:**
`native_x_renderer_unscaled_screen_fractions = false` in `ghvanhalen.toml`.

## The render scale in the shaders' constants (NativeXRenderer D128, 2026-11-08)

With the SDK's recomp change 73, the translated shaders read the render scale from the draw's system constants instead
of having it written into them. Before, a window size, internal resolution or FSR 1 / NIS setting not used before gave
the driver new shaders to compile, and its pipeline cache kept a copy of each pipeline per scale; now one translation
serves every scale. The first start after the change compiles every pipeline once. A draw computes what it computed
(GH Metallica's and the other games' stored translations checked, NativeXRenderer's decisions.md D128).
**Off:** `--native_x_renderer_scale_in_constants=false` writes the scale into the shaders as before.

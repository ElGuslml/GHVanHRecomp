# NativeXRenderer in Guitar Hero: Van Halen

The game's side of NativeXRenderer (`native_x_renderer/`, the library that hooks a game's Direct3D calls and draws
on the host). The hooks and the profile are in
[`src/native_x_renderer_ghvanhalen.cpp`](../src/native_x_renderer_ghvanhalen.cpp); the library's checklist is
`native_x_renderer/docs/integrating-a-game.md`.

**Status (2026-10-26):** GH Metallica's side, translated. Both games link the same XDK Direct3D library (2.0.7978)
and the same engine, so GH Metallica's map (GHMetalRecomp's `docs/native-x-renderer-ghmetallica.md`: the device
layout, Dante's Inferno's up to the register shadow; the current objects POTCAWE's + 144; the resource objects
POTCAWE's) holds here; only the addresses differ.

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

# The fixes ported from GH Metallica

**Status: working (2026-10-26).** Guitar Hero: Van Halen is Guitar Hero: Metallica's engine (the executable
`gh4_3_Final.exe`, Metallica's `gh4_1_burn_Final.exe`; both XDK 2.0.7978): its first boot had GH Metallica's problems
(five cores busy in the menus, among others). GHMetalRecomp's fixes were copied with their names changed and every
address translated: a function matcher over both executables' instruction indexes found each hooked function, and every
hook lands on the same instruction (two of them with a different structure offset in that instruction: the place, not
the registers it reads). Data moved by `+0x1FE80` in `.data` and about `+0x1FDD0` in `.rdata`.

Checked here: the menus and a song at 60 FPS with sound, the CPU, the song clock, quitting from the main menu,
ultrawide on both renderers; Gustavo tried it (2026-10-26). Each section says what the fix does and how to turn
it off; the investigations (the measurements, the evidence) are in GHMetalRecomp's `docs/` and `CLAUDE.md`.

## The threads' waits (CPU)

**Symptom:** about five cores at 100 % in the menus (4.9 measured here). **Cause:** the game's threads wait by checking
a condition again and again, which costs nothing on the console: the job system's three workers (an empty queue,
`sub_8243AF38`), the main thread at the start of each frame (`sub_822F6410`, `sub_822F2A98`), the render thread waiting
for the vertical blank (`sub_82431C58`), the main thread waiting for a group of jobs (`sub_822CC700`), and the D3D
library's waits for the GPU (`sub_821D7378`). **Fix:** `src/thread_waits.cpp`, seven hooks: a wait spins briefly, then
sleeps; the workers block on the job event the game pulses, the render thread sleeps until shortly before the vertical
blank it waits for. Measured: the menus 0.35 cores, a song 1.06 cores at 60 FPS. **Off:** `ghvanhalen_wait_smart =
false` (slices only), `ghvanhalen_wait_spin_us` / `ghvanhalen_wait_sleep_us`.

The generated code also keeps the registers in local variables (the codegen's `*_as_local` options, as GH Metallica:
setjmp `sub_82541C40` and longjmp `sub_82541700`, used only by libjpeg and libpng, share their registers;
non-volatile vector registers stay in the context because this compiler passes values in them).

## The audio drifts out of sync during a song

**Cause:** the engine's timer divides the timebase by its frequency, replacing 50,000,000 by 49,875,000 (the old
consoles' real timebase); ReXGlue's timebase runs at 50 MHz, so the game's clock ran 0.25 % fast and the audio fell
behind the notes. **Fix:** `src/timer.cpp`, a hook in `sub_822E2C58` (0x822E2CF8) keeps the reported frequency.
**Off:** `ghvanhalen_timer_fix = false`. `ghvanhalen.toml` also sets `audio_maxqframes = 4`, the SDK's shortest output
queue (21 ms less audio delay).

## Noise instead of music, and the songs' tracks

The game's audio library (FMOD) mixes when its read position moves to the next block of a short ring, and the songs are
several stereo streams interleaved packet by packet in one XMA file. Both are SDK options set in `ghvanhalen.toml`:
`audio_callback_min_interval_us = 3000` (the audio output called at the console's pace; without it the menu music is
noise) and the SDK's XMA decoder carrying each packet's skip count into the next buffer (always on; without it only the
guitar and the kick drum are heard).

## The guitar and the keyboard

- The game tells controllers apart by their XInput subtype: `gamepad_subtype = 6` reports a gamepad as a guitar
  (with a gamepad's subtype the game asks for a microphone).
- A guitar's whammy bar reads 0 until it is first moved, which the game takes as 43 % pressed:
  `guitar_whammy_rest = true` reports it at rest until then.
- The keyboard as a guitar (`mnk_mode`): frets D F J K L, strum the arrow keys, Star Power Space; GH Van Halen
  Launcher's Keyboard page changes them. Its switch to strum on each fret (`mnk_tap_target = "dpad_down"`) is active
  only in a song (`src/keyboard.cpp`, a hook at `sub_82148478`).

## A second controller

Start on a controller other than the first left an empty screen: the game asks that controller to sign in
(`XamShowSigninUI`) and waited for it forever. `signin_ui_cancel = true` answers as a cancel does on the console: the
game shows its "not signed in" message, and Continue without saving goes on.

## Frame rates above 60

**Experimental.** `ghvanhalen_frame_rate` (60, 120, 144, 160, 165, 240; the launcher's frame rate row) sets the
refresh rate the game paces itself by; its clock is real time, so the songs keep their speed. What counted frames
assuming 60 a second is corrected by `src/framerate.cpp` (`ghvanhalen_frame_rate_fix`, on): the vblank clocks
(`sub_824C7020`, `sub_822E2A80`), the scripts' `Wait N gameframes` (`sub_8243F418`, `sub_8243F468`, `sub_8243F768`), the
button presses the scripts poll every few frames (`sub_823AE218`, the script function ControllerMake `sub_823C97D8`),
the whammy on a sustain (`sub_82153B88`, `sub_82156AF8`, `sub_82156CD8`: 5 frames become 5/60 s) and the loading
screens' spinning icons (`sub_824D5508`, `sub_8240DAA0`, `sub_824D5880`). **Off:** 60 in the launcher.

## Ultrawide

The same as GH Metallica's (`src/widescreen.cpp`, 8 hooks): every 16:9 perspective is widened by the screen's factor
when the engine builds it (`sub_823688F8`, Hor+), and the 2D layer (sprites, text, the highway's board, the tails, the
hit sparks, the loading screens, the clip windows) is narrowed to 16:9 in the center; outside a song black bars cover
the sides. With NativeXRenderer the image is rendered at the screen's aspect ratio. **Off:** 16:9 in the launcher
(`ghvanhalen_ultrawide = false`); `ghvanhalen_ultrawide_hud = false` stretches the 2D layer; `ghvanhalen_menu_bars =
false` shows the room behind the menus at their sides.

## Quitting from the main menu

B on the title screen or the main menu asks whether to quit (A quits, B cancels): `src/quit_prompt.cpp`, a hook in the
main thread's frame (`sub_822F6410`) looks up the title's and the main menu's screen elements (the same names as GH
Metallica's) in the element table at `0x82ACF978`.

## Language

The game has no language menu: it picks its text pack from `XGetLanguage` (French, German, Italian, Spanish, English
otherwise). The launcher's language row sets it.

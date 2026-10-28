# The fixes ported from GH Metallica

**Status: working (2026-10-26; GH Metallica's of 2026-10-27 and 28 added on 2026-10-28: the profile following the
controller that starts the game, the screen effects' switches, the PC microphone, the menu bars beside the image).** Guitar Hero: Van Halen is Guitar Hero: Metallica's engine (the executable
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

Since 2026-10-28 (GH Metallica's of 2026-10-26) the profile follows the controller that starts the game, as on the
console: while the title screen is shown (`src/players.cpp`, from the quit prompt's frame hook that finds the title's
text), the SDK swaps the controller that presses a button with the first one, so it plays with the profile and its
saves (the log: "The controller in slot 1 pressed first: it is now slot 0, the profile's"). The other controllers are
the profile's guests (`signin_guests = true`, "Name (1)"): signed in, with the game's default settings and no saves.
**Off:** `--ghvanhalen_profile_follows_press=false` (the first controller connected plays with the profile),
`signin_guests = false` (the "not signed in" message again).

## Frame rates above 60

**Experimental.** `ghvanhalen_frame_rate` (60, 120, 144, 160, 165, 240; the launcher's frame rate row) sets the
refresh rate the game paces itself by; its clock is real time, so the songs keep their speed. What counted frames
assuming 60 a second is corrected by `src/framerate.cpp` (`ghvanhalen_frame_rate_fix`, on): the vblank clocks
(`sub_824C7020`, `sub_822E2A80`), the scripts' `Wait N gameframes` (`sub_8243F418`, `sub_8243F468`, `sub_8243F768`), the
button presses the scripts poll every few frames (`sub_823AE218`, the script function ControllerMake `sub_823C97D8`),
the whammy on a sustain (`sub_82153B88`, `sub_82156AF8`, `sub_82156CD8`: 5 frames become 5/60 s) and the loading
screens' spinning icons (`sub_824D5508`, `sub_8240DAA0`, `sub_824D5880`). Star power's drain (script `0x74B936AB`)
is not held to 60 passes a second, as it takes the frame's time off the meter each frame: held, star power lasted
160 / 60 times as long at 160 FPS (Gustavo, 2026-10-27). **Off:** 60 in the launcher.

## The crowd's claps on star power

The crowd claps on the beat while star power is on, but late by the player's audio calibration (the music moves with
it, the claps do not; also on the console). The claps come from a script run on each fretbar event, which waits 0.18 s
first: `src/crowd.cpp` (a hook in the script function Wait, `sub_823A5518`) takes the calibration off that wait, read
from the game's globals (0x9D1315C9 less 0xEFEE50D3). Checked in GH Metallica by Gustavo (104 ms of calibration); here
the same scripts and globals. **Off:** `ghvanhalen_crowd_claps_fix = false`.

## Ultrawide

The same as GH Metallica's (`src/widescreen.cpp`, 8 hooks): every 16:9 perspective is widened by the screen's factor
when the engine builds it (`sub_823688F8`, Hor+), and the 2D layer (sprites, text, the highway's board, the tails, the
hit sparks, the loading screens, the clip windows) is narrowed to 16:9 in the center; outside a song black bars cover
the sides. With NativeXRenderer the image is rendered at the screen's aspect ratio, or at another one (16:9, 21:9,
32:9; NativeXRenderer D104, `native_x_renderer_aspect`, the launcher's image format), and the menus' bars are placed
at the sides of the image, not of the window. **Off:** 16:9 in the launcher
(`ghvanhalen_ultrawide = false`); `ghvanhalen_ultrawide_hud = false` stretches the 2D layer; `ghvanhalen_menu_bars =
false` shows the room behind the menus at their sides.

## Turning off the screen effects

**Options, all on by default** (GH Metallica's of 2026-10-28): the engine's screen effects work on the 3D image (the
note highway and the HUD are drawn after them), and its final composite (`sub_825311C8`, the same code as GH
Metallica's) applies six of them in one pass, by a mask with a bit each. `src/screen_fx.cpp` (a hook at `0x825313F0`)
takes the bits of the effects turned off out of that mask: `ghvanhalen_fx_bloom`, `_depth_of_field`, `_grain`,
`_color`, `_vignette` and `_ink` (the ink blotches a song adds in GH Metallica; not seen in this game yet). With the
color correction off the grain is off too (that pass also tones it down). GH Van Halen Launcher's switches are under
Settings → Graphics; the F4 settings overlay applies them at once.

## Singing with a PC microphone

**Experimental, not tried in this game yet** (GH Metallica's of 2026-10-28, where Gustavo sang and scored). The game
looks for an Xbox 360 USB microphone; the SDK answers with a recording device of the PC (`usb_microphone_1`, SDK
change 55; `usb_microphone_1_gain = -12` in `ghvanhalen.toml`, as a PC microphone at full level also hears the room),
and the singer's controller gets an ordinary gamepad's subtype, which the game gives the vocals, while the others are
guitars (`gamepad_device_subtypes`, change 56). GH Van Halen Launcher's Controls tab sets both: the Microphone under
the keyboard (device, channel, gain, level) and each controller's Instrument row (Guitar or Vocals). **Off:**
`usb_microphone_1 = ""` (the default: the launcher's Microphone: None).

## Quitting from the main menu

B on the title screen or the main menu asks whether to quit (A quits, B cancels): `src/quit_prompt.cpp`, a hook in the
main thread's frame (`sub_822F6410`) looks up the title's and the main menu's screen elements (the same names as GH
Metallica's) in the element table at `0x82ACF978`.

## Language

The game has no language menu: it picks its text pack from `XGetLanguage` (French, German, Italian, Spanish, English
otherwise). The launcher's language row sets it.

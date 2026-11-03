# Documentation

Short notes on each problem found while making the game run, and what was done about it. Each one gives
the symptom, the cause, the fix and how to turn the fix off, so that you can check it on your own machine.

Guitar Hero: Van Halen runs on the same engine as Guitar Hero: Metallica (its executable is `gh4_3_Final.exe`,
Metallica's `gh4_1_burn_Final.exe`) with the same XDK, so it has the same problems. GHMetalRecomp's fixes were ported
with every address translated; the investigations are GHMetalRecomp's.

To build the project and work on it: [Building and developing](development.md).

| Note | Status |
|---|---|
| [The fixes ported from GH Metallica](fixes-from-gh-metallica.md) | Working: the threads' waits, the song clock, the audio, the guitar, the keyboard and a controller as a guitar, the second controller (the profile follows the controller that starts the game), frame rates above 60, ultrawide, the screen effects' switches, quitting from the main menu; singing with a PC microphone experimental |
| [NativeXRenderer in Guitar Hero: Van Halen](native-x-renderer-ghvanhalen.md) | Working: GH Metallica's map and profile, translated (the default renderer since 0.1.0) |

# Documentation

Short notes on each problem found while making the game run, and what was done about it. Each one gives
the symptom, the cause, the fix and how to turn the fix off, so that you can check it on your own machine.

Guitar Hero: Van Halen runs on the same engine as Guitar Hero: Metallica (its executable is `gh4_3_Final.exe`,
Metallica's `gh4_1_burn_Final.exe`) with the same XDK, so it has the same problems. GHMetalRecomp's fixes were ported
with every address translated; the investigations are GHMetalRecomp's.

| Note | Status |
|---|---|
| [The fixes ported from GH Metallica](fixes-from-gh-metallica.md) | Working: the threads' waits, the song clock, the audio, the guitar and the keyboard, the second controller, frame rates above 60, ultrawide, quitting from the main menu |
| [NativeXRenderer in Guitar Hero: Van Halen](native-x-renderer-ghvanhalen.md) | Working: GH Metallica's map and profile, translated (off by default) |

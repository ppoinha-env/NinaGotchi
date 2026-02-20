NinaBot — Detailed Step-by-step Sound Integration Spec

Purpose
-------
Provide an exact, implementable step-by-step specification to integrate the DF1201S audio player used in `SoundCYDLittleFS.ino` into the main PlatformIO app (`src/main.cpp`). This is a specification only — do NOT change any code now. Follow these steps when ready to implement.

Assumptions / fixed choices
--------------------------
- Use the startup filename you specified: `heninapersrob.mp3` (play at app start).
- Use the serial port pins exactly as in `SoundCYDLittleFS.ino`: `Serial2.begin(115200, SERIAL_8N1, 27, 22);` (RX=27, TX=22).
- Audio filenames (exact strings to use):
  - Learning letters: `a.mp3` .. `z.mp3`
  - Learning numbers: `0.mp3` .. `10.mp3`
  - Startup: `heninapersrob.mp3`
  - Eating: `yumtasty.mp3`
  - Heart (petting): `ailveunina.mp3`
  - Rain: `onorain.mp3`
  - Fright/shake: `awishap.mp3`
- Files are stored where the DF1201S expects (root of SD or root of FS). Paths in code will use leading slash, e.g. `"/a.mp3"`.

Deliverables (files to add)
---------------------------
- `platformio.ini` change: add DFPlayer library dependency (registry or GitHub URL). Example entry:

  lib_deps =
    DFRobot/DFRobot_DF1201S

  If not available in registry use:

  lib_deps =
    https://github.com/DFRobot/DFRobot_DF1201S.git

- Add audio wrapper files:
  - `src/audio.h`
  - `src/audio.cpp`

- Update `specs.md` (already present) and add this `SPECS_DETAILED.md` for implementer reference.

Audio wrapper API (required functions and behavior)
--------------------------------------------------
Create a small, well-documented wrapper around `DFRobot_DF1201S` implementing these functions:

- `bool audio_begin()`
  - Initialize `Serial2` exactly with `Serial2.begin(115200, SERIAL_8N1, 27, 22);`
  - Call `DF1201S.begin(Serial2)`; return `true` on success or `false` after retries.

- `void audio_loop()`
  - Non-blocking poll routine called each `loop()` iteration (if needed to update internal state such as `isPlaying` by sampling `getCurTime()`).

- `bool audio_play(const char *path, bool force=false)`
  - Start playback of the specified file path (e.g. `"/a.mp3"`).
  - If `force==false` do nothing when the same file is currently playing.
  - Return `true` if playback started, `false` if skipped.

- `void audio_play_once(const char *path)`
  - Convenience wrapper that calls `audio_play(path, true)` then returns immediately.

- `bool audio_is_playing()`
  - Returns `true` if DF1201S reports a playback time > 0 (i.e., playing), otherwise `false`.

- `void audio_stop()`
  - Stop current playback.

- Internals / debounce
  - Keep `String lastPlayedPath` and `unsigned long lastPlayMs`.
  - Do not retrigger the same path if it was started in the last N ms (suggest 1200 ms) or if `audio_is_playing()` returns `true` unless `force==true`.
  - Provide debug `Serial` messages when a file starts/stops (optional for testing).

Where to call audio functions (exact insertion points)
-----------------------------------------------------
All changes below are *additions* — they do not modify the existing logic, only call the audio wrapper at the appropriate moments.

1) Initialization (in `setup()`)
- Location: in `src/main.cpp` inside `setup()` after `touch.begin(touchSPI); touch.setRotation(1);` and after `faceSprite.createSprite(...)` but before `lastTouchTime = millis();`.
- Actions:
  - Add `#include "audio.h"` at the top of `main.cpp`.
  - Call `audio_begin()`.
  - Start the startup sound once: `audio_play_once("/heninapersrob.mp3");`

  Example (pseudo-code to insert):

  // after existing init code
  audio_begin();
  audio_play_once("/heninapersrob.mp3");

2) Regular polling (in `loop()`)
- Location: inside the main `loop()` function, ideally at the top or bottom of `loop()` so it runs every iteration.
- Actions:
  - Call `audio_loop();` each loop iteration to let the wrapper update its internal state.

3) Learning mode — play when value changes
- Preferred insertion point: Immediately after `generateRandomValue()` is called and `lastValueChange` is updated.
- There are two places where `generateRandomValue()` is used:
  - When switching to learning mode in the sidebar touch handler:
    - After `generateRandomValue(); lastValueChange = now;` add audio trigger.
  - In the timed update where learning mode updates value every 3000 ms:
    - After `generateRandomValue(); lastValueChange = now;` add audio trigger.

- Mapping rule:
  - If `currentLearningValue` is a single alphabetic character, map to lowercase filename: `toLowerCase(currentLearningValue.charAt(0)) + ".mp3"`. E.g. `"A"` -> `"/a.mp3"`.
  - If it is numeric (including `10`), map to `"/<value>.mp3"` (e.g. `"10"` -> `"/10.mp3"`).

- Call: `audio_play(mappedPath);`
- Do not block the UI while waiting for the audio to finish.

4) Feeding animation — play `yumtasty.mp3`
- In the touch handler where feeding is triggered:
  - When code sets `isFeeding = true; cookieX = 0; feedingStartTime = now;` add `audio_play("/yumtasty.mp3");` immediately after the `isFeeding` is set.
  - This ensures playback starts at the start of the eating animation.

5) Petting (heart) animation — play `ailveunina.mp3`
- In the touch handler where petting is triggered:
  - Where code sets `isPetting = true; isDizzy = false; animEndTime = now + 3000; currentActiveBtn = 1;` add `audio_play("/ailveunina.mp3");`.
  - Optionally, use the wrapper check to avoid retrigger if the sound is already playing.

6) Rain animation — play `onorain.mp3`
- The visual "rain" effect is implemented in `drawFace()` under `case 3:` of `switch(currentMood)`.
- Where to trigger: play when `currentMood` becomes `3` (i.e., at the point where `currentMood` is set). There are two places `currentMood` is set:
  - When updating mood periodically: `currentMood = random(0, 20); lastMoodChange = now;`
  - Optionally when explicitly set elsewhere (not present now).
- Insert a small check immediately after the `currentMood = random(0, 20);` assignment:

  if (currentMood == 3) audio_play("/onorain.mp3");

  (Wrap with wrapper debounce so it does not play repeatedly when mood remains 3.)

7) Fright / shake (dizzy) animation — play `awishap.mp3`
- In the touch handler where dizzy is triggered:
  - After `isDizzy = true; isPetting = false; animEndTime = now + 3000;` add `audio_play("/awishap.mp3");`.

Debounce & non-retrigger policy (important)
-------------------------------------------
- Use the audio wrapper's `audio_is_playing()` and `lastPlayedPath` to avoid playing the same file repeatedly.
- For event sounds (petting, eating, rain, fright) use a small cooldown window (e.g., 1200 ms) so the same event retriggering within that window won't restart the same file.
- For learning-mode mapping, allow retriggers when the value actually changes (i.e., when `lastValueChange` is updated), otherwise prevent spurious repeats.

Filesystem & file paths
-----------------------
- Use leading slash paths (example: `"/a.mp3"`) to match the style in `SoundCYDLittleFS.ino`.
- Ensure all files are placed in the SD root (or FS root) exactly with the names above.
- Validate using a simple manual test: insert a test call `audio_play("/a.mp3");` and confirm playback.

PlatformIO additions
--------------------
- Add DF1201S library dependency to `platformio.ini`. Example:

  lib_deps =
    https://github.com/DFRobot/DFRobot_DF1201S.git

- No other `build_flags` needed unless DF1201S requires macros.

Implementation checklist (developer steps)
-----------------------------------------
1. Add `lib_deps` entry and run a PlatformIO build to download libraries.
2. Add `src/audio.h` / `src/audio.cpp` implementing the wrapper API described above.
3. Add `#include "audio.h"` to `src/main.cpp`.
4. In `setup()` call `audio_begin();` then `audio_play_once("/heninapersrob.mp3");`.
5. Add `audio_loop();` to the top or bottom of `loop()`.
6. Insert `audio_play()` calls at the exact points described where `generateRandomValue()` is called and where animations are triggered (feeding, petting, dizzy, rain mood change).
7. Implement debounce logic in `audio.cpp` and test on device.
8. Place all MP3 files in the SD root (or the appropriate location DF1201S expects). Confirm filenames and case.
9. Test all triggers and iterate.

Testing procedure
-----------------
1. Build project in PlatformIO: `platformio run --environment esp32dev`.
2. Upload to device: `platformio run -t upload --environment esp32dev`.
3. Open serial monitor: `platformio device monitor -e esp32dev` (or use `platformio.ini` monitor settings). Confirm audio wrapper debug messages.
4. Verify `heninapersrob.mp3` plays on startup.
5. Switch to learning mode; ensure each generated letter/number plays the expected file.
6. Trigger feeding, petting (heart), rain mood, and dizzy — check correct file plays for each.
7. Confirm UI remains responsive while audio plays.
8. If playback fails, verify SD card contents and file paths.

Edge cases & notes
------------------
- If `DFRobot_DF1201S` provides a blocking API `playSpecFile()` that doesn't return immediately, you must switch to the non-blocking polling approach using `getCurTime()` as in the original `SoundCYDLittleFS.ino` `playAndWait()` example.
- If file names are different on the target device, update mapping in `main.cpp` calls accordingly.

Questions / Clarifications
-------------------------
1. Confirm that all sound filenames listed above are exact and are present on the SD/FS (case-sensitive). You specified `heninapersrob.mp3` — I will use that exact name in the spec and code calls.
2. Confirm the DF1201S wiring uses `Serial2` with pins RX=27, TX=22 — spec uses exactly these pins.

End of specification

This document contains a precise, implementable step sequence and code insertion points. When you're ready, I can implement the wrapper and the callsites for you; until then, no code will be changed.
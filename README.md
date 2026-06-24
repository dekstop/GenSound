# GenSynth

A macOS AU/VST3 instrument plugin for fast sketching of generative and tracker-friendly instruments in ordinary C++.

Write a small C++ script, save it, and the plugin hot-reloads the compiled result while the DAW keeps running.

---

## Requirements

- macOS (Apple Silicon or Intel)
- Xcode command-line tools (`xcode-select --install`)
- CMake 3.22+
- A DAW that loads AU or VST3 plugins

JUCE 8.x is fetched automatically by CMake.

---

## Building

```bash
git clone <this-repo> GenSynth
cd GenSynth
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j$(sysctl -n hw.logicalcpu)
```

The built plugin appears in `build/GenSynth_artefacts/Release/`.

Copy it to your plugin folder:
```bash
# AU
cp -r build/GenSynth_artefacts/Release/AU/GenSynth.component ~/Library/Audio/Plug-Ins/Components/

# VST3
cp -r build/GenSynth_artefacts/Release/VST3/GenSynth.vst3 ~/Library/Audio/Plug-Ins/VST3/
```

---

## Writing a Script

1. Create a `.cpp` file anywhere on disk.
2. Include the SDK header:

```cpp
#include "GenSynthSDK.h"
```

3. Export two functions:

```cpp
GS_EXPORT void initVoice(VoiceState* vs)
{
    // Zero-initialise your state struct here.
    // Called once when a note-on triggers a new voice.
}

GS_EXPORT VoiceOutput synth(VoiceState* vs, const VoiceContext* ctx)
{
    // Produce one stereo sample per call.
    // Return alive=false to end the voice.
    return { leftSample, rightSample, alive };
}
```

4. In the plugin UI, drag the `.cpp` file into the plugin window (or click **Choose Script**).
5. The plugin compiles and loads it. Any subsequent saves trigger an automatic recompile.

---

## VoiceState

256 bytes of per-voice scratch memory, zero-filled before `initVoice`. Use `gs::stateAs<T>()` to cast it:

```cpp
struct MyState { float phase; float env; };

GS_EXPORT void initVoice(VoiceState* vs)
{
    auto* s = gs::stateAs<MyState>(vs);
    s->phase = 0.0f;
    s->env   = 0.0f;
}
```

`sizeof(MyState)` must be ≤ 256. A `static_assert` in `gs::stateAs` will catch violations at script compile time.

---

## VoiceContext fields

| Field         | Type      | Description                                        |
|---------------|-----------|----------------------------------------------------|
| `sampleRate`  | `float`   | e.g. 44100.0                                       |
| `freq`        | `float`   | Fundamental frequency in Hz                        |
| `phase`       | `float`   | Normalised phase [0, 1), advanced by host          |
| `t`           | `float`   | Seconds since note-on                              |
| `note`        | `uint8_t` | MIDI note number                                   |
| `velocity`    | `uint8_t` | MIDI velocity                                      |
| `released`    | `bool`    | True after note-off                                |
| `releaseTime` | `float`   | Seconds since note-off (0 while held)              |
| `bpm`         | `float`   | Host tempo                                         |
| `beat`        | `float`   | Host beat position (quarter-note units)            |
| `songTime`    | `float`   | Host position in seconds                           |
| `p[8]`        | `float[]` | Plugin parameters, mapped to the 8 knobs           |
| `seed`        | `uint32_t`| Stable deterministic seed for this voice           |

---

## SDK helpers (`gs::` namespace)

```cpp
// Oscillators
gs::sine(phase)           // sine wave
gs::saw(phase)            // sawtooth
gs::square(phase, pw)     // square / pulse
gs::tri(phase)            // triangle

// Noise
gs::noise(rng)            // white noise in [-1, 1], mutates rng

// Envelopes
gs::adsr(t, released, releaseTime, A, D, S, R)
gs::expDecay(t, halfLife)

// Pitch
gs::noteToHz(note)
gs::semisToRatio(semitones)

// Filter
gs::OnePole lp;
lp.process(sample, gs::lpCoeff(cutoffHz, sampleRate))

// Clipping
gs::softClip(x)
gs::clamp(x, lo, hi)

// State cast
gs::stateAs<MyState>(vs)
```

---

## Example Scripts

| File             | Description                                  |
|------------------|----------------------------------------------|
| `simple_sine.cpp`| Sine oscillator with ADSR and lowpass filter |
| `kick.cpp`       | Sine sweep kick with click transient         |
| `fm_bass.cpp`    | Two-operator FM bass                         |
| `pluck.cpp`      | Karplus-Strong string (notes ≥ E4 at 44100)  |
| `snare.cpp`      | Noise + tonal snare                          |
| `gated_pad.cpp`  | Beat-synchronised gated pad with filter sweep|

---

## Plugin UI

- **Choose Script** — file browser to select a `.cpp` source file.
- **Compile** — manually trigger compilation (auto-triggered on file save).
- **Open Folder** — reveal the script folder in Finder.
- Status line shows compile result, active version number, and voice count.
- Error output box displays the raw clang diagnostics.
- Drag a `.cpp` file directly onto the plugin window to load it.

---

## Parameters

Eight unlabelled knobs (`P0`–`P7`) are available as `ctx->p[0]`–`ctx->p[7]` in scripts. Map them to whatever your script needs. They are automatable in the DAW.

---

## Voice versioning

When you save a script and it recompiles successfully:

- Newly triggered notes use the new dylib.
- Notes that were already playing continue using the dylib they started with.
- Old dylibs are unloaded automatically once no voices reference them.
- Failed compilations never affect currently playing audio.

---

## Milestones implemented

- [x] M1 — JUCE plugin shell
- [x] M2 — Polyphonic voice engine
- [x] M3 — External compilation pipeline
- [x] M4 — Loader and versioning (hot-swap)
- [x] M5 — File watching and compile button
- [x] M6 — Transport-aware scripting, diagnostics, example instruments
- [ ] M7 — Offline render/export (future)

---

## Notes and caveats

**AU code signing** — AU plugins require a valid code signature to load in Logic Pro and GarageBand. Sign with your Apple Developer identity:
```bash
codesign --force --sign "Developer ID Application: Your Name (TEAMID)" \
  build/GenSynth_artefacts/Release/AU/GenSynth.component
```
VST3 works unsigned for testing in most DAWs.

**Karplus-Strong pitch range** — `pluck.cpp` uses a fixed-size delay line (60 samples). This limits minimum pitch to roughly E4 at 44100 Hz. For lower pitches, reduce the fundamental frequency mapping or increase `kMaxDelay` (stays within 256 bytes up to ~64 samples).

**Script ABI version** — `GENSYNTH_ABI_VERSION` in `GenSynthSDK.h` is currently `1`. The host and scripts must match. If the ABI changes, bump the version and recompile all scripts.

**No sandbox** — scripts run in-process. This is intentional for the demoscene/sizecoding use case where the user is also the script author.

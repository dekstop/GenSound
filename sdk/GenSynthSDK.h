#pragma once

//==============================================================================
// GenSynthSDK.h
// Stable ABI between GenSynth host and user-authored instrument scripts.
//
// Rules for scripts:
//   1. Include this header only — never include JUCE or host internals.
//   2. Export initVoice() and synth() with extern "C".
//   3. Store per-voice state in the VoiceState scratch block.
//   4. Signal voice end by returning VoiceOutput with alive = false.
//
// ABI version history:
//   v1 — initial release
//==============================================================================

#include <cstdint>
#include <cmath>
#include <cstring>

//------------------------------------------------------------------------------
// ABI version — bump if layout changes
//------------------------------------------------------------------------------
static constexpr uint32_t GENSYNTH_ABI_VERSION = 1;

//------------------------------------------------------------------------------
// VoiceState
// 256 bytes of host-allocated, script-owned scratch memory per voice.
// Persists for the full lifetime of the voice. Zero-initialised by the host
// before initVoice() is called. Scripts cast this to their own struct.
//------------------------------------------------------------------------------
struct VoiceState
{
    alignas(8) uint8_t data[256];
};

//------------------------------------------------------------------------------
// VoiceContext
// Read-only context passed to synth() on every sample. All values are set by
// the host. Scripts must not write to this struct.
//------------------------------------------------------------------------------
struct VoiceContext
{
    // Synthesis fundamentals
    float    sampleRate;      // e.g. 44100.0, 48000.0
    float    freq;            // fundamental frequency in Hz (derived from note + pitchBend)
    float    phase;           // normalised phase [0, 1), advanced by host each sample
    float    t;               // time in seconds since note-on

    // MIDI / performance
    uint8_t  note;            // MIDI note number (0–127)
    uint8_t  velocity;        // MIDI velocity (0–127)
    uint8_t  _pad0[2];

    // Release
    bool     released;        // true once note-off received
    float    releaseTime;     // seconds since note-off (0 while held)
    uint8_t  _pad1[3];

    // Transport
    float    bpm;             // host tempo in BPM
    float    beat;            // current beat position (e.g. 1.5 = halfway through beat 2)
    float    songTime;        // song position in seconds

    // Parameters — up to 8 floats from plugin UI or host automation
    float    p[8];

    // Deterministic seed for noise / random use — stable per voice lifetime
    uint32_t seed;

    uint8_t  _pad2[4];
};

//------------------------------------------------------------------------------
// VoiceOutput
// Returned by synth() each sample.
//------------------------------------------------------------------------------
struct VoiceOutput
{
    float left;    // left channel sample
    float right;   // right channel sample
    bool  alive;   // set to false to signal voice end; host will deactivate
};

//==============================================================================
// Script export macros
//==============================================================================
#ifdef __cplusplus
#  define GS_EXPORT extern "C" __attribute__((visibility("default")))
#else
#  define GS_EXPORT __attribute__((visibility("default")))
#endif

// Required exports — every script must define both of these:
//
//   GS_EXPORT void      initVoice (VoiceState* vs);
//   GS_EXPORT VoiceOutput synth   (VoiceState* vs, const VoiceContext* ctx);

//==============================================================================
// Utility helpers (header-only, available to all scripts)
//==============================================================================
namespace gs
{

// --- Oscillators ------------------------------------------------------------

inline float sine (float phase)
{
    return std::sin (phase * 6.283185307f);
}

inline float saw (float phase)
{
    return 2.0f * (phase - std::floor (phase + 0.5f));
}

inline float square (float phase, float pw = 0.5f)
{
    float p = phase - std::floor (phase);
    return p < pw ? 1.0f : -1.0f;
}

inline float tri (float phase)
{
    float p = phase - std::floor (phase);
    return p < 0.5f ? 4.0f * p - 1.0f : 3.0f - 4.0f * p;
}

// --- Noise ------------------------------------------------------------------

// Simple xorshift32 — fast, deterministic, reasonable quality
inline uint32_t xorshift32 (uint32_t& state)
{
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return state;
}

// White noise in [-1, 1]
inline float noise (uint32_t& state)
{
    return static_cast<float> (xorshift32 (state)) / 2147483648.0f - 1.0f;
}

// --- Envelopes --------------------------------------------------------------

// Linear ADSR — returns gain [0, 1]
// All times in seconds. Call each sample with current t and releaseTime.
inline float adsr (float t, float released, float releaseTime,
                   float attack, float decay, float sustain, float release)
{
    if (!released)
    {
        if (t < attack)
            return t / attack;
        t -= attack;
        if (t < decay)
            return 1.0f - (1.0f - sustain) * (t / decay);
        return sustain;
    }
    // release phase
    float env = sustain * (1.0f - releaseTime / release);
    return env > 0.0f ? env : 0.0f;
}

// Simple exponential decay — useful for percussion
inline float expDecay (float t, float halfLife)
{
    return std::exp2 (-t / halfLife);
}

// --- Pitch / frequency utilities --------------------------------------------

inline float noteToHz (int note)
{
    return 440.0f * std::exp2 ((note - 69) / 12.0f);
}

inline float semisToRatio (float semitones)
{
    return std::exp2 (semitones / 12.0f);
}

// --- Mixing / clipping ------------------------------------------------------

inline float clamp (float x, float lo, float hi)
{
    return x < lo ? lo : (x > hi ? hi : x);
}

inline float softClip (float x)
{
    // tanh-approximation, gain-staged to stay near ±1
    float a = x < -3.0f ? -1.0f : (x > 3.0f ? 1.0f : x * (27.0f + x * x) / (27.0f + 9.0f * x * x));
    return a;
}

// --- Filter -----------------------------------------------------------------

// One-pole lowpass — update coeff only when cutoff changes
struct OnePole
{
    float z = 0.0f;

    float process (float x, float coeff)
    {
        z = z + coeff * (x - z);
        return z;
    }
};

// Compute lowpass coefficient from cutoff Hz and sampleRate
inline float lpCoeff (float cutoffHz, float sampleRate)
{
    float omega = 6.283185307f * cutoffHz / sampleRate;
    return omega / (omega + 1.0f);
}

// --- State helper -----------------------------------------------------------

// Cast VoiceState scratch to user struct — use inside initVoice / synth
template<typename T>
inline T* stateAs (VoiceState* vs)
{
    static_assert (sizeof(T) <= sizeof(VoiceState::data),
                   "Script state struct exceeds VoiceState::data (256 bytes)");
    static_assert (alignof(T) <= 8,
                   "Script state struct alignment exceeds VoiceState guarantee (8 bytes)");
    return reinterpret_cast<T*> (vs->data);
}

} // namespace gs

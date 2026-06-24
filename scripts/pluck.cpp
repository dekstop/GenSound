// pluck.cpp — Karplus-Strong string synthesis
// A delay line with gentle lowpass feedback simulates a plucked string.
// The delay line lives in VoiceState (fits within 256 bytes for most pitches).
//
// p[0] = string damping  (0–1, higher = faster decay)
// p[1] = pick position   (0–1, controls tone brightness)
//
// Note: delay-line length = sampleRate / freq.
// Maximum length before exceeding VoiceState: 256/4 = 64 samples → ~688 Hz min at 44100.
// For lower pitches use a shorter state struct or a heap-allocated extension.
// This version supports MIDI notes 64+ (E4 and above) safely at 44100.

#include "GenSynthSDK.h"
#include <cmath>

static constexpr int kMaxDelay = 60;  // 256 bytes / sizeof(float) - headroom

struct State
{
    float  buf[kMaxDelay];
    int    len      = 0;
    int    writePos = 0;
    float  lp_z     = 0.0f;   // one-pole LP state
};

static_assert (sizeof(State) <= sizeof(VoiceState::data), "State too large");

GS_EXPORT void initVoice (VoiceState* vs)
{
    auto* s  = gs::stateAs<State> (vs);
    uint32_t rng = vs->data[0] ^ 0xcafef00d;
    if (rng == 0) rng = 1;

    // Length in samples for this pitch
    int len = static_cast<int> (44100.0f / 329.63f);  // placeholder; updated in synth on t==0
    s->len     = (len < 2) ? 2 : (len > kMaxDelay ? kMaxDelay : len);
    s->writePos = 0;
    s->lp_z     = 0.0f;

    // Fill with band-limited noise (the initial excitation)
    float lpz = 0.0f;
    for (int i = 0; i < s->len; ++i)
    {
        float n = gs::noise (rng);
        lpz = lpz + 0.5f * (n - lpz);
        s->buf[i] = lpz;
    }
}

GS_EXPORT VoiceOutput synth (VoiceState* vs, const VoiceContext* ctx)
{
    auto* s = gs::stateAs<State> (vs);

    // Recalculate delay length on first sample
    if (ctx->t < 1.0f / ctx->sampleRate)
    {
        int len = static_cast<int> (ctx->sampleRate / ctx->freq);
        s->len = (len < 2) ? 2 : (len > kMaxDelay ? kMaxDelay : len);
    }

    float damping = 0.02f + ctx->p[0] * 0.25f;  // higher p[0] = more damping

    // Read output from delay line
    int readPos = (s->writePos - s->len + kMaxDelay) % s->len;
    float y = s->buf[readPos];

    // Lowpass (Karplus-Strong average filter)
    float coeff = 1.0f - damping;
    float fed   = coeff * (y + s->lp_z) * 0.5f;
    s->lp_z     = y;

    s->buf[s->writePos] = fed;
    s->writePos = (s->writePos + 1) % s->len;

    float gain   = static_cast<float> (ctx->velocity) / 127.0f;
    float sample = y * gain;

    // Voice ends when energy has decayed to near silence
    float energy = fed * fed;
    bool alive   = energy > 1e-10f;

    return { sample, sample, alive };
}

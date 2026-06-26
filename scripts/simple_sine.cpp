// simple_sine.cpp — basic sine oscillator with ADSR envelope
// Build: the GenSound plugin compiles this automatically.

#include "GenSoundSDK.h"

struct State
{
    gs::OnePole  filter;
    float        filterCutoff = 0.0f;
};

GS_EXPORT void initVoice (VoiceState* vs)
{
    auto* s = gs::stateAs<State> (vs);
    s->filter.z = 0.0f;
}

GS_EXPORT VoiceOutput synth (VoiceState* vs, const VoiceContext* ctx)
{
    auto* s = gs::stateAs<State> (vs);

    // p[0] = filter cutoff (0–1 → 200–8000 Hz)
    float cutoffHz  = 200.0f + ctx->p[0] * 7800.0f;
    float coeff     = gs::lpCoeff (cutoffHz, ctx->sampleRate);

    // p[1] = release time (0–1 → 0.05–2 s)
    float rel = 0.05f + ctx->p[1] * 1.95f;

    float env = gs::adsr (ctx->t, ctx->released, ctx->releaseTime,
                           0.01f, 0.1f, 0.7f, rel);

    float sample = gs::sine (ctx->phase) * env;
    sample = s->filter.process (sample, coeff);

    // Normalise by velocity
    float gain = static_cast<float> (ctx->velocity) / 127.0f;
    sample *= gain;

    bool alive = !(ctx->released && ctx->releaseTime > rel + 0.01f);

    return { sample, sample, alive };
}

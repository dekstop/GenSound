// fm_bass.cpp — two-operator FM bass
// Operator 2 modulates Operator 1.
// p[0] = modulation index  (0–1 → 0–8)
// p[1] = mod ratio         (0–1 → 0.5–4.0)
// p[2] = attack            (0–1 → 0.001–0.1 s)
// p[3] = release           (0–1 → 0.05–1.5 s)

#include "GenSoundSDK.h"

struct State
{
    float carrierPhase   = 0.0f;
    float modulatorPhase = 0.0f;
};

GS_EXPORT void initVoice (VoiceState* vs)
{
    auto* s = gs::stateAs<State> (vs);
    s->carrierPhase   = 0.0f;
    s->modulatorPhase = 0.0f;
}

GS_EXPORT VoiceOutput synth (VoiceState* vs, const VoiceContext* ctx)
{
    auto* s = gs::stateAs<State> (vs);

    float modIdx   = ctx->p[0] * 8.0f;
    float modRatio = 0.5f + ctx->p[1] * 3.5f;
    float attack   = 0.001f + ctx->p[2] * 0.099f;
    float rel      = 0.05f  + ctx->p[3] * 1.45f;

    float env = gs::adsr (ctx->t, ctx->released, ctx->releaseTime,
                           attack, 0.08f, 0.6f, rel);

    float carrierFreq   = ctx->freq;
    float modulatorFreq = carrierFreq * modRatio;

    // Advance phases
    s->modulatorPhase += modulatorFreq / ctx->sampleRate;
    if (s->modulatorPhase >= 1.0f) s->modulatorPhase -= 1.0f;

    // FM modulation: phase modulate the carrier by the modulator
    float modSignal = gs::sine (s->modulatorPhase) * modIdx;
    float carrierPhaseMod = s->carrierPhase + modSignal / gs::TWO_PI;

    s->carrierPhase += carrierFreq / ctx->sampleRate;
    if (s->carrierPhase >= 1.0f) s->carrierPhase -= 1.0f;

    float sample = gs::sine (carrierPhaseMod) * env;
    sample = gs::softClip (sample * 1.2f);

    float gain = static_cast<float> (ctx->velocity) / 127.0f;
    sample    *= gain;

    bool alive = !(ctx->released && ctx->releaseTime > rel + 0.02f);

    return { sample, sample, alive };
}

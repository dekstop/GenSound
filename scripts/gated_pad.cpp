// gated_pad.cpp — beat-synchronised gated pad
// The pad is rhythmically gated based on beat position and a selectable pattern.
// p[0] = gate rate   (0=1/4 note, 0.25=1/8, 0.5=1/16, 0.75=1/32)
// p[1] = gate duty   (0–1, open fraction of gate period)
// p[2] = filter sweep depth (0–1)

#include "GenSoundSDK.h"
#include <cmath>

struct State
{
    gs::OnePole lp1;
    gs::OnePole lp2;  // two-pole lowpass approximation
    float       detunePhase = 0.0f;
};

GS_EXPORT void initVoice (VoiceState* vs)
{
    auto* s = gs::stateAs<State> (vs);
    s->lp1.z        = 0.0f;
    s->lp2.z        = 0.0f;
    s->detunePhase  = 0.0f;
}

GS_EXPORT VoiceOutput synth (VoiceState* vs, const VoiceContext* ctx)
{
    auto* s = gs::stateAs<State> (vs);

    // Gate division: p[0] selects 1/4, 1/8, 1/16, 1/32 note subdivisions
    int divSteps = 1 << static_cast<int> (ctx->p[0] * 3.99f);  // 1, 2, 4, 8
    float divBeats   = 1.0f / static_cast<float> (divSteps);
    float beatInDiv  = std::fmod (ctx->beat, divBeats);
    float gatePhase  = beatInDiv / divBeats;  // [0, 1) within gate period
    float duty       = 0.05f + ctx->p[1] * 0.9f;
    float gate       = (gatePhase < duty) ? 1.0f : 0.0f;

    // Simple amplitude envelope to soften gate edges
    float gateEnv = gs::expDecay (gatePhase < duty ? gatePhase / duty
                                                    : (gatePhase - duty) / (1.0f - duty),
                                  0.05f);
    float gateAmp = (gatePhase < duty) ? (1.0f - gateEnv * 0.2f) : 0.0f;

    // Slightly detuned saw + sine for rich pad texture
    s->detunePhase += (ctx->freq * 1.005f) / ctx->sampleRate;
    if (s->detunePhase >= 1.0f) s->detunePhase -= 1.0f;

    float osc = gs::saw (ctx->phase) * 0.6f
              + gs::sine (s->detunePhase) * 0.4f;

    // Filter sweep driven by beat position
    float sweep    = std::sin (ctx->beat * gs::PI * 0.5f) * ctx->p[2];
    float cutoffHz = 400.0f + sweep * 4000.0f;
    float coeff    = gs::lpCoeff (cutoffHz, ctx->sampleRate);

    float filtered = s->lp1.process (osc, coeff);
    filtered       = s->lp2.process (filtered, coeff);

    float sample  = filtered * gateAmp;
    float gain    = static_cast<float> (ctx->velocity) / 127.0f;
    sample       *= gain * 0.7f;

    // Held notes stay alive; release fades quickly
    float env  = ctx->released ? gs::expDecay (ctx->releaseTime, 0.05f) : 1.0f;
    sample    *= env;

    bool alive = !(ctx->released && ctx->releaseTime > 0.3f);
    return { sample, sample, alive };
}

// snare.cpp — noise-based snare with tonal body + noise component
// p[0] = tone / noise blend  (0=all tone, 1=all noise)
// p[1] = decay time          (0–1 → 0.05–0.4 s)

#include "GenSynthSDK.h"

struct State
{
    float     bodyPhase = 0.0f;
    uint32_t  rng       = 1;
    gs::OnePole hpLP;   // simple HP approximation using LP complement
    gs::OnePole bodyLP;
};

GS_EXPORT void initVoice (VoiceState* vs)
{
    auto* s = gs::stateAs<State> (vs);
    s->bodyPhase = 0.0f;
    s->rng       = vs->data[0] ^ 0x12345678;
    if (s->rng == 0) s->rng = 1;
    s->hpLP.z    = 0.0f;
    s->bodyLP.z  = 0.0f;
}

GS_EXPORT VoiceOutput synth (VoiceState* vs, const VoiceContext* ctx)
{
    auto* s = gs::stateAs<State> (vs);

    float decay = 0.05f + ctx->p[1] * 0.35f;
    float blend = ctx->p[0];  // 0 = tonal, 1 = noisy

    float env = gs::expDecay (ctx->t, decay);

    // Tonal body — two sine waves a fifth apart
    s->bodyPhase += ctx->freq / ctx->sampleRate;
    if (s->bodyPhase >= 1.0f) s->bodyPhase -= 1.0f;
    float body  = gs::sine (s->bodyPhase) * 0.5f;
    body       += gs::sine (s->bodyPhase * 1.5f) * 0.3f;
    body        = s->bodyLP.process (body, gs::lpCoeff (900.0f, ctx->sampleRate));

    // Noise component — bandpass approximation (LP at 8k, HP via complement at 1k)
    float raw   = gs::noise (s->rng);
    float lp    = s->hpLP.process (raw, gs::lpCoeff (8000.0f, ctx->sampleRate));
    float hp    = raw - s->hpLP.process (raw, gs::lpCoeff (1000.0f, ctx->sampleRate));
    float noise = (lp + hp) * 0.5f;

    float sample = (body * (1.0f - blend) + noise * blend) * env;
    float gain   = static_cast<float> (ctx->velocity) / 127.0f;
    sample      *= gain;

    bool alive = ctx->t < decay * 6.0f;
    return { sample, sample, alive };
}

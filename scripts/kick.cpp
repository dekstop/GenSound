// kick.cpp — punchy kick drum using sine sweep + click transient
// Pitch: MIDI note sets base tuning (usually C1 = 36)
// p[0] = pitch sweep depth (0–1)
// p[1] = body decay (0–1 → 0.1–0.6 s)

#include "GenSoundSDK.h"

struct State
{
    float phase = 0.0f;
    uint32_t rng;
};

GS_EXPORT void initVoice (VoiceState* vs)
{
    auto* s = gs::stateAs<State> (vs);
    s->phase = 0.0f;
    s->rng   = vs->data[0] ^ 0xdeadbeef;  // seed from VoiceState bytes
    if (s->rng == 0) s->rng = 1;
}

GS_EXPORT VoiceOutput synth (VoiceState* vs, const VoiceContext* ctx)
{
    auto* s = gs::stateAs<State> (vs);

    float bodyDecay  = 0.1f + ctx->p[1] * 0.5f;
    float sweepDepth = 1.0f + ctx->p[0] * 3.0f;

    // Exponential pitch sweep from sweepDepth * baseFreq down to baseFreq
    float baseFreq  = ctx->freq;
    float freqNow   = baseFreq * sweepDepth * gs::expDecay (ctx->t, 0.025f)
                    + baseFreq * (1.0f - gs::expDecay (ctx->t, 0.025f));

    // Phase accumulation (manual, ignoring host phase for pitch accuracy)
    s->phase += freqNow / ctx->sampleRate;
    if (s->phase >= 1.0f) s->phase -= 1.0f;

    float body  = gs::sine (s->phase) * gs::expDecay (ctx->t, bodyDecay);

    // Short click transient
    float click = gs::noise (s->rng) * gs::expDecay (ctx->t, 0.003f) * 0.5f;

    float sample = gs::softClip ((body + click) * 0.9f);
    float gain   = static_cast<float> (ctx->velocity) / 127.0f;
    sample      *= gain;

    bool alive = ctx->t < bodyDecay * 6.0f;

    return { sample, sample, alive };
}

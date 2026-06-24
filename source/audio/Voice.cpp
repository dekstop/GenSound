#include "Voice.h"
#include <cmath>
#include <cstring>
#include <cassert>

//==============================================================================
static float noteToHz (uint8_t note)
{
    return 440.0f * std::exp2f ((static_cast<float>(note) - 69.0f) / 12.0f);
}

//==============================================================================
Voice::~Voice()
{
    deactivate();
}

//------------------------------------------------------------------------------
void Voice::activate (uint8_t                        note,
                      uint8_t                        velocity,
                      float                          sampleRate,
                      std::shared_ptr<ScriptVersion> scriptVersion,
                      uint32_t                       seed)
{
    assert (scriptVersion && scriptVersion->valid);

    // Release any previous version before overwriting
    if (scriptVersion_)
        scriptVersion_->release();

    note_          = note;
    velocity_      = velocity;
    sampleRate_    = sampleRate;
    freq_          = noteToHz (note);
    phase_         = 0.0f;
    t_             = 0.0f;
    releaseTime_   = 0.0f;
    released_      = false;
    seed_          = seed;
    active_        = true;
    scriptVersion_ = std::move (scriptVersion);

    // Zero state and call script init
    std::memset (&state_, 0, sizeof (state_));
    scriptVersion_->initVoice (&state_);
    scriptVersion_->addRef();
}

//------------------------------------------------------------------------------
void Voice::release()
{
    if (!active_ || released_) return;
    released_ = true;
}

//------------------------------------------------------------------------------
bool Voice::renderSample (float  bpm,
                          float  beat,
                          float  songTime,
                          const float p[8],
                          float& outL,
                          float& outR)
{
    if (!active_) return false;

    VoiceContext ctx {};
    ctx.sampleRate  = sampleRate_;
    ctx.freq        = freq_;
    ctx.phase       = phase_;
    ctx.t           = t_;
    ctx.note        = note_;
    ctx.velocity    = velocity_;
    ctx.released    = released_;
    ctx.releaseTime = releaseTime_;
    ctx.bpm         = bpm;
    ctx.beat        = beat;
    ctx.songTime    = songTime;
    ctx.seed        = seed_;
    for (int i = 0; i < 8; ++i)
        ctx.p[i] = p[i];

    VoiceOutput out = scriptVersion_->synth (&state_, &ctx);

    outL += out.left;
    outR += out.right;

    // Advance phase — wrap within [0, 1)
    float phaseInc = freq_ / sampleRate_;
    phase_ = phase_ + phaseInc;
    if (phase_ >= 1.0f) phase_ -= 1.0f;

    // Advance time
    float dt = 1.0f / sampleRate_;
    t_ += dt;
    if (released_) releaseTime_ += dt;

    if (!out.alive)
    {
        deactivate();
        return false;
    }

    return true;
}

//------------------------------------------------------------------------------
void Voice::deactivate()
{
    if (scriptVersion_)
    {
        scriptVersion_->release();
        scriptVersion_.reset();
    }
    active_   = false;
    released_ = false;
}

//------------------------------------------------------------------------------
uint32_t Voice::versionNumber() const
{
    if (scriptVersion_) return scriptVersion_->versionNumber;
    return 0;
}

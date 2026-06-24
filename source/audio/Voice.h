#pragma once

#include "../scripting/ScriptingTypes.h"
#include "../loader/ScriptVersion.h"
#include <memory>
#include <cstdint>

//==============================================================================
// Voice
//
// Represents one active (or recently released) synthesis voice. The audio
// thread owns all Voice objects; no other thread may write to them.
//
// Lifetime:
//   1. Host calls activate() when a note-on arrives.
//   2. Host calls renderSample() once per sample from the audio callback.
//   3. Host calls release() on note-off.
//   4. renderSample() returns false (or alive is false) → voice is inactive.
//   5. Host calls deactivate() to reclaim the slot.
//==============================================================================
class Voice
{
public:
    Voice() = default;
    ~Voice();

    //--------------------------------------------------------------------------
    // Activate this voice slot for a new note.
    // scriptVersion must be non-null and valid.
    void activate (uint8_t                        note,
                   uint8_t                        velocity,
                   float                          sampleRate,
                   std::shared_ptr<ScriptVersion> scriptVersion,
                   uint32_t                       seed);

    // Signal note-off. Voice continues rendering until it reports alive=false.
    void release();

    // Render one sample into left/right. Returns false when the voice is done.
    // Must only be called when isActive() == true.
    bool renderSample (float  bpm,
                       float  beat,
                       float  songTime,
                       const float p[8],
                       float& outL,
                       float& outR);

    // Mark the voice as inactive and release the script version reference.
    void deactivate();

    //--------------------------------------------------------------------------
    bool     isActive()   const { return active_; }
    bool     isReleased() const { return released_; }
    uint8_t  note()       const { return note_; }
    uint8_t  velocity()   const { return velocity_; }
    uint32_t versionNumber() const;

private:
    bool     active_      = false;
    bool     released_    = false;
    uint8_t  note_        = 0;
    uint8_t  velocity_    = 0;
    float    sampleRate_  = 44100.0f;
    float    phase_       = 0.0f;
    float    freq_        = 440.0f;
    float    t_           = 0.0f;       // seconds since note-on
    float    releaseTime_ = 0.0f;       // seconds since note-off
    uint32_t seed_        = 1;

    VoiceState                     state_ {};
    std::shared_ptr<ScriptVersion> scriptVersion_;
};

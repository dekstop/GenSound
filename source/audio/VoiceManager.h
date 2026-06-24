#pragma once

#include "Voice.h"
#include "../loader/ScriptVersion.h"
#include <array>
#include <memory>
#include <atomic>
#include <mutex>

//==============================================================================
// VoiceManager
//
// Manages a fixed pool of Voice objects. Called exclusively from the audio
// thread (renderBlock, handleNoteOn, handleNoteOff).
//
// The current ScriptVersion is published to the audio thread via an atomic
// pointer (setCurrentScriptVersion / currentScriptVersion_). The background
// thread writes it; the audio thread reads it on the next note-on.
//==============================================================================
static constexpr int kMaxVoices = 16;

class VoiceManager
{
public:
    VoiceManager();

    //--------------------------------------------------------------------------
    // Audio-thread interface

    // Process a block of samples. Mixes all active voices into outL / outR.
    // outL and outR must be pre-zeroed by the caller.
    void renderBlock (float*       outL,
                      float*       outR,
                      int          numSamples,
                      float        sampleRate,
                      float        bpm,
                      float        beatAtBlockStart,
                      float        beatsPerSample,
                      float        songTimeAtBlockStart,
                      const float  p[8]);

    // Handle MIDI note-on. Called from audio thread with a valid scriptVersion.
    void handleNoteOn  (uint8_t note, uint8_t velocity);
    void handleNoteOff (uint8_t note);

    int activeVoiceCount() const;

    //--------------------------------------------------------------------------
    // Cross-thread: set the script version for newly triggered notes.
    // Safe to call from background thread; read on audio thread at note-on.
    void setCurrentScriptVersion (std::shared_ptr<ScriptVersion> version);
    std::shared_ptr<ScriptVersion> getCurrentScriptVersion() const;

private:
    std::array<Voice, kMaxVoices> voices_;

    // Voice allocation
    int  findFreeVoice()  const;
    int  stealVoice()     const;   // oldest released voice, then oldest held

    // Seed counter for deterministic per-voice noise
    uint32_t nextSeed_ = 1;

    // Published from background thread, read at note-on on audio thread
    mutable std::mutex               versionMutex_;
    std::shared_ptr<ScriptVersion>   currentVersion_;

    float sampleRate_ = 44100.0f;
};

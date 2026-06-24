#include "VoiceManager.h"
#include <cassert>
#include <limits>

//==============================================================================
VoiceManager::VoiceManager() = default;

//------------------------------------------------------------------------------
void VoiceManager::setCurrentScriptVersion (std::shared_ptr<ScriptVersion> version)
{
    std::lock_guard<std::mutex> lk (versionMutex_);
    currentVersion_ = std::move (version);
}

std::shared_ptr<ScriptVersion> VoiceManager::getCurrentScriptVersion() const
{
    std::lock_guard<std::mutex> lk (versionMutex_);
    return currentVersion_;
}

//------------------------------------------------------------------------------
int VoiceManager::findFreeVoice() const
{
    for (int i = 0; i < kMaxVoices; ++i)
        if (!voices_[i].isActive())
            return i;
    return -1;
}

int VoiceManager::stealVoice() const
{
    // Prefer a released voice (about to end anyway)
    for (int i = 0; i < kMaxVoices; ++i)
        if (voices_[i].isActive() && voices_[i].isReleased())
            return i;

    // Fall back to voice 0 (oldest by index — good enough for now)
    return 0;
}

//------------------------------------------------------------------------------
void VoiceManager::handleNoteOn (uint8_t note, uint8_t velocity)
{
    if (velocity == 0)
    {
        handleNoteOff (note);
        return;
    }

    auto version = getCurrentScriptVersion();
    if (!version || !version->valid)
        return;   // no script loaded yet — silently ignore

    int slot = findFreeVoice();
    if (slot < 0)
        slot = stealVoice();

    if (voices_[slot].isActive())
        voices_[slot].deactivate();

    voices_[slot].activate (note, velocity, sampleRate_, version, nextSeed_++);
}

void VoiceManager::handleNoteOff (uint8_t note)
{
    for (auto& v : voices_)
        if (v.isActive() && !v.isReleased() && v.note() == note)
            v.release();
}

//------------------------------------------------------------------------------
void VoiceManager::renderBlock (float*      outL,
                                 float*      outR,
                                 int         numSamples,
                                 float       sampleRate,
                                 float       bpm,
                                 float       beatAtBlockStart,
                                 float       beatsPerSample,
                                 float       songTimeAtBlockStart,
                                 const float p[8])
{
    sampleRate_ = sampleRate;

    for (int s = 0; s < numSamples; ++s)
    {
        float beat     = beatAtBlockStart + beatsPerSample * static_cast<float>(s);
        float songTime = songTimeAtBlockStart + static_cast<float>(s) / sampleRate;

        float l = 0.0f, r = 0.0f;

        for (auto& v : voices_)
        {
            if (!v.isActive()) continue;

            float vl = 0.0f, vr = 0.0f;
            bool alive = v.renderSample (bpm, beat, songTime, p, vl, vr);
            (void) alive;  // deactivation handled inside renderSample

            l += vl;
            r += vr;
        }

        outL[s] = l;
        outR[s] = r;
    }
}

//------------------------------------------------------------------------------
int VoiceManager::activeVoiceCount() const
{
    int count = 0;
    for (const auto& v : voices_)
        if (v.isActive()) ++count;
    return count;
}

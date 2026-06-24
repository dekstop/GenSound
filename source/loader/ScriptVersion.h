#pragma once

#include "../scripting/ScriptingTypes.h"
#include <atomic>
#include <cstdint>
#include <string>

//==============================================================================
// ScriptVersion
//
// Owns a single loaded .dylib and the resolved function pointers.
// Reference-counted: the host adds a ref when a voice starts and releases it
// when the voice ends. The dylib is unloaded only when refCount reaches zero
// and the version has been superseded (superseded flag set by ScriptManager).
//
// Thread-safety:
//   refCount is modified atomically from the audio thread (addRef / release).
//   All other fields are written once at construction and read-only thereafter.
//==============================================================================

using InitVoiceFn  = void        (*)(VoiceState*);
using SynthFn      = VoiceOutput (*)(VoiceState*, const VoiceContext*);

struct ScriptVersion
{
    //--------------------------------------------------------------------------
    uint32_t      versionNumber  = 0;
    std::string   dylibPath;        // absolute path to the .dylib on disk
    void*         handle           = nullptr;  // dlopen handle
    InitVoiceFn   initVoice        = nullptr;
    SynthFn       synth            = nullptr;
    bool          valid            = false;    // true if both symbols resolved

    std::atomic<int> refCount { 0 };
    std::atomic<bool> superseded { false };    // set by ScriptManager when a newer version loads

    // Non-copyable, non-movable — managed through shared_ptr
    ScriptVersion()  = default;
    ~ScriptVersion() = default;
    ScriptVersion (const ScriptVersion&) = delete;
    ScriptVersion& operator= (const ScriptVersion&) = delete;

    //--------------------------------------------------------------------------
    void addRef()   { refCount.fetch_add (1, std::memory_order_relaxed); }
    void release()  { refCount.fetch_sub (1, std::memory_order_release); }
    int  refs() const { return refCount.load (std::memory_order_acquire); }

    bool canUnload() const
    {
        return superseded.load (std::memory_order_acquire)
               && refs() == 0;
    }
};

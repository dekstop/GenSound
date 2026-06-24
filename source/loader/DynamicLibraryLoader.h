#pragma once

#include "ScriptVersion.h"
#include <memory>
#include <string>

//==============================================================================
// DynamicLibraryLoader
//
// Loads a versioned .dylib, resolves initVoice and synth, and returns a
// populated ScriptVersion. Call from a background thread only.
//==============================================================================
class DynamicLibraryLoader
{
public:
    // Attempt to load the dylib at path and resolve required symbols.
    // Returns a valid ScriptVersion on success, nullptr on failure.
    // errorOut receives a human-readable message on failure.
    static std::shared_ptr<ScriptVersion> load (const std::string& dylibPath,
                                                uint32_t           versionNumber,
                                                std::string&       errorOut);

    // Unload a dylib if it has no active voice references and is superseded.
    // Safe to call repeatedly; is a no-op if conditions are not met.
    static void tryUnload (std::shared_ptr<ScriptVersion>& version);
};

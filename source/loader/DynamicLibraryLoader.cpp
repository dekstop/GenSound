#include "DynamicLibraryLoader.h"
#include <dlfcn.h>

std::shared_ptr<ScriptVersion>
DynamicLibraryLoader::load (const std::string& dylibPath,
                             uint32_t           versionNumber,
                             std::string&       errorOut)
{
    // Clear any stale dlerror state
    dlerror();

    void* handle = dlopen (dylibPath.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (handle == nullptr)
    {
        const char* err = dlerror();
        errorOut = err ? err : "dlopen returned null with no error message";
        return nullptr;
    }

    auto sv = std::make_shared<ScriptVersion>();
    sv->versionNumber = versionNumber;
    sv->dylibPath     = dylibPath;
    sv->handle        = handle;

    dlerror();
    sv->initVoice = reinterpret_cast<InitVoiceFn> (dlsym (handle, "initVoice"));
    if (sv->initVoice == nullptr)
    {
        const char* err = dlerror();
        errorOut = std::string ("Failed to resolve 'initVoice': ") + (err ? err : "unknown");
        dlclose (handle);
        return nullptr;
    }

    dlerror();
    sv->synth = reinterpret_cast<SynthFn> (dlsym (handle, "synth"));
    if (sv->synth == nullptr)
    {
        const char* err = dlerror();
        errorOut = std::string ("Failed to resolve 'synth': ") + (err ? err : "unknown");
        dlclose (handle);
        return nullptr;
    }

    sv->valid = true;
    return sv;
}

void DynamicLibraryLoader::tryUnload (std::shared_ptr<ScriptVersion>& version)
{
    if (version == nullptr)
        return;

    if (version->canUnload() && version->handle != nullptr)
    {
        dlclose (version->handle);
        version->handle    = nullptr;
        version->initVoice = nullptr;
        version->synth     = nullptr;
        version->valid     = false;
        version.reset();
    }
}

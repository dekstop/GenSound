#pragma once

#include "../compile/ExternalCompiler.h"
#include "../compile/CompileDiagnostics.h"
#include "../loader/DynamicLibraryLoader.h"
#include "../loader/ScriptVersion.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <mutex>
#include <atomic>

//==============================================================================
// ScriptManager
//
// High-level owner of the compile → load → version lifecycle.
// Lives on the main/UI thread. Compilation occurs on a worker thread.
// The active ScriptVersion is pushed to VoiceManager via a callback.
//==============================================================================
class ScriptManager
{
public:
    // Called on the compiler thread after each compile attempt (success or fail).
    using OnCompileComplete = std::function<void (const CompileDiagnostics& diag,
                                                  std::shared_ptr<ScriptVersion> newVersion)>;

    ScriptManager();
    ~ScriptManager();

    //--------------------------------------------------------------------------
    // Configuration — call before compiling
    void setScriptPath      (const std::string& path);
    void setBuildDirectory  (const std::string& dir);
    void setSdkIncludePath  (const std::string& path);

    const std::string& scriptPath() const { return scriptPath_; }

    //--------------------------------------------------------------------------
    // Trigger a compile. Non-blocking. Callback fires when done.
    void compile (OnCompileComplete callback);

    bool isCompiling() const { return compiler_.isCompiling(); }

    //--------------------------------------------------------------------------
    // Current active version (set after successful load)
    std::shared_ptr<ScriptVersion> activeVersion() const;
    uint32_t                       activeVersionNumber() const;

    //--------------------------------------------------------------------------
    // Housekeeping — call periodically from the UI/main thread to unload
    // superseded dylibs whose reference count has dropped to zero.
    void collectGarbage();

private:
    void onCompileDone (uint32_t                 versionNumber,
                        const std::string&       dylibPath,
                        const CompileDiagnostics& diag,
                        OnCompileComplete         callback);

    std::string scriptPath_;
    std::string buildDir_;
    std::string sdkIncludePath_;

    std::atomic<uint32_t> nextVersionNumber_ { 1 };

    ExternalCompiler compiler_;

    mutable std::mutex             activeVersionMutex_;
    std::shared_ptr<ScriptVersion> activeVersion_;

    std::mutex                                  oldVersionsMutex_;
    std::vector<std::shared_ptr<ScriptVersion>> oldVersions_;
};

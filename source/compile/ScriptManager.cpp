#include "ScriptManager.h"
#include <cassert>

//==============================================================================
ScriptManager::ScriptManager()  = default;
ScriptManager::~ScriptManager() = default;

//------------------------------------------------------------------------------
void ScriptManager::setScriptPath     (const std::string& path) { scriptPath_      = path; }
void ScriptManager::setBuildDirectory (const std::string& dir)  { buildDir_        = dir;
                                                                    compiler_.setOutputDirectory (dir); }
void ScriptManager::setSdkIncludePath (const std::string& path) { sdkIncludePath_  = path;
                                                                    compiler_.setSdkIncludePath (path); }

//------------------------------------------------------------------------------
void ScriptManager::compile (OnCompileComplete callback)
{
    if (scriptPath_.empty())
        return;

    uint32_t ver = nextVersionNumber_.fetch_add (1);

    compiler_.compileAsync (
        scriptPath_,
        ver,
        [this, cb = std::move (callback)]
        (uint32_t vn, const std::string& dylibPath, const CompileDiagnostics& diag)
        {
            onCompileDone (vn, dylibPath, diag, cb);
        });
}

//------------------------------------------------------------------------------
void ScriptManager::onCompileDone (uint32_t                  versionNumber,
                                   const std::string&        dylibPath,
                                   const CompileDiagnostics& diag,
                                   OnCompileComplete         callback)
{
    std::shared_ptr<ScriptVersion> newVersion;

    if (diag.success && !dylibPath.empty())
    {
        std::string loadError;
        newVersion = DynamicLibraryLoader::load (dylibPath, versionNumber, loadError);

        if (!newVersion)
        {
            // Treat load failure as a compile-like failure — synthesise a diagnostics
            CompileDiagnostics loadDiag;
            loadDiag.success   = false;
            loadDiag.rawStderr = "dylib load failed: " + loadError;
            if (callback) callback (loadDiag, nullptr);
            return;
        }

        // Supersede and archive the old version
        {
            std::lock_guard<std::mutex> lk (activeVersionMutex_);
            if (activeVersion_)
            {
                activeVersion_->superseded.store (true);
                std::lock_guard<std::mutex> lkOld (oldVersionsMutex_);
                oldVersions_.push_back (std::move (activeVersion_));
            }
            activeVersion_ = newVersion;
        }
    }

    if (callback) callback (diag, newVersion);
}

//------------------------------------------------------------------------------
std::shared_ptr<ScriptVersion> ScriptManager::activeVersion() const
{
    std::lock_guard<std::mutex> lk (activeVersionMutex_);
    return activeVersion_;
}

uint32_t ScriptManager::activeVersionNumber() const
{
    std::lock_guard<std::mutex> lk (activeVersionMutex_);
    return activeVersion_ ? activeVersion_->versionNumber : 0;
}

//------------------------------------------------------------------------------
void ScriptManager::collectGarbage()
{
    std::lock_guard<std::mutex> lk (oldVersionsMutex_);
    oldVersions_.erase (
        std::remove_if (oldVersions_.begin(), oldVersions_.end(),
                        [] (std::shared_ptr<ScriptVersion>& v)
                        {
                            if (v && v->canUnload())
                            {
                                DynamicLibraryLoader::tryUnload (v);
                                return true;
                            }
                            return false;
                        }),
        oldVersions_.end());
}

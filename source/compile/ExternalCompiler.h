#pragma once

#include "CompileDiagnostics.h"
#include <functional>
#include <string>
#include <atomic>
#include <thread>
#include <mutex>

//==============================================================================
// ExternalCompiler
//
// Invokes clang++ as a subprocess to compile a script source file into a
// versioned .dylib. Must be used from a background thread; never call compile()
// from the audio thread.
//
// Usage:
//   ExternalCompiler compiler;
//   compiler.setOutputDirectory ("/tmp/gensound_build");
//   compiler.setSdkIncludePath  ("/path/to/sdk");
//   compiler.compileAsync ("kick.cpp", [] (uint32_t ver, const std::string& path,
//                                         const CompileDiagnostics& diag) { ... });
//==============================================================================
class ExternalCompiler
{
public:
    ExternalCompiler();
    ~ExternalCompiler();

    //--------------------------------------------------------------------------
    // Configuration — set before compiling
    void setOutputDirectory (const std::string& dir);
    void setSdkIncludePath  (const std::string& path);
    void setExtraFlags      (const std::string& flags);   // appended verbatim

    //--------------------------------------------------------------------------
    // Compile a source file asynchronously.
    // The callback is invoked on the compiler thread, NOT the audio thread.
    //   versionNumber — the next version number to use for the output dylib
    //   dylibPath     — absolute path to the produced .dylib (empty on failure)
    //   diag          — parsed diagnostics
    using CompileCallback = std::function<void (uint32_t              versionNumber,
                                                const std::string&    dylibPath,
                                                const CompileDiagnostics& diag)>;

    // Enqueues a compilation. If a build is already running, the new request
    // replaces any pending (not-yet-started) request so that rapid edits
    // coalesce into a single build.
    void compileAsync (const std::string& sourcePath,
                       uint32_t           nextVersion,
                       CompileCallback    callback);

    bool isCompiling() const { return compiling_.load(); }

    //--------------------------------------------------------------------------
    // Synchronous compile — for tests or one-shot CLI use
    CompileDiagnostics compileSync (const std::string& sourcePath,
                                    uint32_t           versionNumber,
                                    std::string&       dylibPathOut);

private:
    std::string buildOutputPath (const std::string& sourcePath, uint32_t version) const;
    int         runProcess      (const std::string& cmdline, std::string& stderrOut);

    std::string outputDir_;
    std::string sdkIncludePath_;
    std::string extraFlags_;

    std::atomic<bool>   compiling_    { false };
    std::atomic<bool>   pendingBuild_ { false };

    struct PendingRequest
    {
        std::string     sourcePath;
        uint32_t        nextVersion = 0;
        CompileCallback callback;
    };

    std::mutex      pendingMutex_;
    PendingRequest  pending_;
    std::thread     workerThread_;
    std::atomic<bool> shutdown_ { false };

    void workerLoop();
    void startWorkerIfNeeded();
};

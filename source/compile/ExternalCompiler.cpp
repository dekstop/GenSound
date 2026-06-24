#include "ExternalCompiler.h"

#include <filesystem>
#include <sstream>
#include <array>
#include <cstdio>
#include <cassert>

namespace fs = std::filesystem;

//==============================================================================
ExternalCompiler::ExternalCompiler()  = default;

ExternalCompiler::~ExternalCompiler()
{
    shutdown_.store (true);
    if (workerThread_.joinable())
        workerThread_.join();
}

//------------------------------------------------------------------------------
void ExternalCompiler::setOutputDirectory (const std::string& dir)
{
    outputDir_ = dir;
    fs::create_directories (dir);
}

void ExternalCompiler::setSdkIncludePath (const std::string& path)
{
    sdkIncludePath_ = path;
}

void ExternalCompiler::setExtraFlags (const std::string& flags)
{
    extraFlags_ = flags;
}

//------------------------------------------------------------------------------
std::string ExternalCompiler::buildOutputPath (const std::string& sourcePath,
                                               uint32_t            version) const
{
    // stem of source file + version number, e.g. kick_0003.dylib
    std::string stem = fs::path (sourcePath).stem().string();
    char buf[64];
    std::snprintf (buf, sizeof (buf), "_%04u.dylib", version);
    return (fs::path (outputDir_) / (stem + buf)).string();
}

//------------------------------------------------------------------------------
// Run a shell command and capture its stderr. Returns the exit code.
int ExternalCompiler::runProcess (const std::string& cmdline,
                                  std::string&       stderrOut)
{
    // Redirect stderr to stdout so we capture it via popen.
    std::string cmd = cmdline + " 2>&1";

    FILE* pipe = popen (cmd.c_str(), "r");
    if (!pipe)
    {
        stderrOut = "popen() failed — cannot launch compiler";
        return -1;
    }

    std::array<char, 512> buf;
    while (fgets (buf.data(), static_cast<int>(buf.size()), pipe) != nullptr)
        stderrOut += buf.data();

    return pclose (pipe);
}

//------------------------------------------------------------------------------
CompileDiagnostics ExternalCompiler::compileSync (const std::string& sourcePath,
                                                   uint32_t           versionNumber,
                                                   std::string&       dylibPathOut)
{
    dylibPathOut.clear();

    std::string outputPath = buildOutputPath (sourcePath, versionNumber);

    // Build the clang++ command
    std::ostringstream cmd;
    cmd << "clang++"
        << " -std=c++20"
        << " -O2"
        << " -dynamiclib"
        << " -fPIC"
        << " -fvisibility=hidden"          // only GS_EXPORT symbols are visible
        << " -fno-exceptions"
        << " -fno-rtti";

    if (!sdkIncludePath_.empty())
        cmd << " -I" << sdkIncludePath_;

    if (!extraFlags_.empty())
        cmd << " " << extraFlags_;

    cmd << " -o \"" << outputPath << "\""
        << " \"" << sourcePath << "\"";

    std::string stderrText;
    int exitCode = runProcess (cmd.str(), stderrText);
    bool ok = (exitCode == 0);

    auto diag = CompileDiagnostics::parse (stderrText, ok);

    if (ok)
        dylibPathOut = outputPath;

    return diag;
}

//------------------------------------------------------------------------------
void ExternalCompiler::compileAsync (const std::string& sourcePath,
                                     uint32_t           nextVersion,
                                     CompileCallback    callback)
{
    {
        std::lock_guard<std::mutex> lk (pendingMutex_);
        pending_.sourcePath  = sourcePath;
        pending_.nextVersion = nextVersion;
        pending_.callback    = std::move (callback);
        pendingBuild_.store (true);
    }

    startWorkerIfNeeded();
}

//------------------------------------------------------------------------------
void ExternalCompiler::startWorkerIfNeeded()
{
    if (compiling_.load())
        return;  // worker is already running; it will pick up the pending request

    if (workerThread_.joinable())
        workerThread_.join();

    workerThread_ = std::thread (&ExternalCompiler::workerLoop, this);
}

//------------------------------------------------------------------------------
void ExternalCompiler::workerLoop()
{
    while (!shutdown_.load())
    {
        // Pick up the pending request (if any)
        PendingRequest req;
        {
            std::lock_guard<std::mutex> lk (pendingMutex_);
            if (!pendingBuild_.load())
                break;   // nothing to do — exit thread

            req = pending_;
            pendingBuild_.store (false);
        }

        compiling_.store (true);

        std::string dylibPath;
        auto diag = compileSync (req.sourcePath, req.nextVersion, dylibPath);

        compiling_.store (false);

        if (req.callback)
            req.callback (req.nextVersion, dylibPath, diag);

        // Check if another request arrived while we were compiling
        if (!pendingBuild_.load())
            break;
    }
}

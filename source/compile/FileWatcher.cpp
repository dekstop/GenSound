#include "FileWatcher.h"
#include <filesystem>

namespace fs = std::filesystem;

void FileWatcher::start (const std::string&            filePath,
                         ChangeCallback                 callback,
                         std::chrono::milliseconds      pollInterval)
{
    stop();
    path_     = filePath;
    callback_ = std::move (callback);
    running_.store (true);
    thread_   = std::thread (&FileWatcher::loop, this, pollInterval);
}

void FileWatcher::stop()
{
    running_.store (false);
    if (thread_.joinable())
        thread_.join();
}

void FileWatcher::loop (std::chrono::milliseconds interval)
{
    fs::file_time_type lastMtime {};

    // Get initial mtime without firing callback
    std::error_code ec;
    if (fs::exists (path_, ec))
        lastMtime = fs::last_write_time (path_, ec);

    while (running_.load())
    {
        std::this_thread::sleep_for (interval);

        if (!running_.load()) break;

        std::error_code ec2;
        if (!fs::exists (path_, ec2))
            continue;

        auto mtime = fs::last_write_time (path_, ec2);
        if (ec2) continue;

        if (mtime != lastMtime)
        {
            lastMtime = mtime;
            if (callback_)
                callback_ (path_);
        }
    }
}

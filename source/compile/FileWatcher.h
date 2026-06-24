#pragma once

#include <string>
#include <functional>
#include <thread>
#include <atomic>
#include <filesystem>
#include <chrono>

//==============================================================================
// FileWatcher
//
// Polls a single file for mtime changes at a configurable interval.
// Fires a callback on a background thread when the file changes.
// Simple stat-based polling — no FSEvents dependency.
//==============================================================================
class FileWatcher
{
public:
    using ChangeCallback = std::function<void (const std::string& path)>;

    FileWatcher()  = default;
    ~FileWatcher() { stop(); }

    // Start watching. Calls callback on background thread when file changes.
    void start (const std::string& filePath,
                ChangeCallback     callback,
                std::chrono::milliseconds pollInterval = std::chrono::milliseconds (500));

    void stop();

    bool isWatching() const { return running_.load(); }
    const std::string& path() const { return path_; }

private:
    void loop (std::chrono::milliseconds interval);

    std::string    path_;
    ChangeCallback callback_;
    std::thread    thread_;
    std::atomic<bool> running_ { false };
};

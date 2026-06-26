#pragma once
#include <string>
#include <vector>

//==============================================================================
// CompileDiagnostics
// Holds the raw output and structured records from a clang++ invocation.
//
// Pass/fail is determined solely by the process exit code (success field),
// not by counting parsed error records — clang emits some diagnostics in
// formats that don't match the structured file:line:col: pattern (e.g.
// fatal errors from missing headers), so counting is unreliable.
//==============================================================================
struct DiagnosticRecord
{
    enum class Kind { Error, Warning, Note, Unknown };
    Kind        kind    = Kind::Unknown;
    std::string file;
    int         line    = 0;
    int         col     = 0;
    std::string message;
    std::string raw;
};

struct CompileDiagnostics
{
    bool                          success      = false;  // true iff clang exit code == 0
    std::string                   rawStderr;             // full clang output for the log
    std::vector<DiagnosticRecord> records;               // parsed structured lines
    int                           warningCount = 0;      // parsed warnings (informational only)

    static CompileDiagnostics parse (const std::string& stderr_text, bool compilationSucceeded);

    // Short one-line summary for the plugin status label.
    // "Compiled OK" / "Compiled OK (N warnings)" / "Compile failed"
    std::string summary() const;
};

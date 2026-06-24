#pragma once
#include <string>
#include <vector>

//==============================================================================
// CompileDiagnostics
// Parses clang stderr into structured records for display.
//==============================================================================
struct DiagnosticRecord
{
    enum class Kind { Error, Warning, Note, Unknown };
    Kind        kind    = Kind::Unknown;
    std::string file;
    int         line    = 0;
    int         col     = 0;
    std::string message;
    std::string raw;    // original line
};

struct CompileDiagnostics
{
    bool                          success = false;
    std::string                   rawStderr;
    std::vector<DiagnosticRecord> records;
    int                           errorCount   = 0;
    int                           warningCount = 0;

    // Parse clang's stderr output into records
    static CompileDiagnostics parse (const std::string& stderr_text, bool compilationSucceeded);

    // Produce a single human-readable summary string for the plugin UI
    std::string summary() const;
};

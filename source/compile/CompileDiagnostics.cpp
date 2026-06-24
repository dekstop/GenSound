#include "CompileDiagnostics.h"
#include <sstream>
#include <regex>

CompileDiagnostics CompileDiagnostics::parse (const std::string& stderr_text,
                                               bool compilationSucceeded)
{
    CompileDiagnostics diag;
    diag.success   = compilationSucceeded;
    diag.rawStderr = stderr_text;

    // clang diagnostic format:
    //   /path/to/file.cpp:12:5: error: some message
    //   /path/to/file.cpp:12:5: warning: some message
    static const std::regex lineRe (
        R"(^([^:]+):(\d+):(\d+):\s+(error|warning|note):\s+(.*)$)");

    std::istringstream ss (stderr_text);
    std::string line;
    while (std::getline (ss, line))
    {
        std::smatch m;
        DiagnosticRecord rec;
        rec.raw = line;

        if (std::regex_match (line, m, lineRe))
        {
            rec.file    = m[1].str();
            rec.line    = std::stoi (m[2].str());
            rec.col     = std::stoi (m[3].str());
            rec.message = m[5].str();

            std::string kind = m[4].str();
            if      (kind == "error")   { rec.kind = DiagnosticRecord::Kind::Error;   ++diag.errorCount; }
            else if (kind == "warning") { rec.kind = DiagnosticRecord::Kind::Warning; ++diag.warningCount; }
            else if (kind == "note")    { rec.kind = DiagnosticRecord::Kind::Note; }
            else                        { rec.kind = DiagnosticRecord::Kind::Unknown; }
        }
        else
        {
            rec.kind    = DiagnosticRecord::Kind::Unknown;
            rec.message = line;
        }

        if (!line.empty())
            diag.records.push_back (std::move (rec));
    }

    return diag;
}

std::string CompileDiagnostics::summary() const
{
    if (success && errorCount == 0 && warningCount == 0)
        return "Compiled OK";

    if (success && errorCount == 0)
        return "Compiled OK (" + std::to_string (warningCount) + " warning"
               + (warningCount == 1 ? "" : "s") + ")";

    std::string s = std::to_string (errorCount) + " error"
                  + (errorCount == 1 ? "" : "s");
    if (warningCount > 0)
        s += ", " + std::to_string (warningCount) + " warning"
           + (warningCount == 1 ? "" : "s");
    return s;
}

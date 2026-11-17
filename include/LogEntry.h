#ifndef LOG_ENTRY_H
#define LOG_ENTRY_H

#include <cstddef>
#include <string>

enum class LogLevel {
    Debug,
    Info,
    Warning,
    Error
};

struct LogEntry {
    std::string timestamp;
    LogLevel level;
    std::string module;
    std::string message;
    std::size_t lineNumber = 0;
};

std::string logLevelToString(LogLevel level);

#endif

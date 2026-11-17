#ifndef LOG_PARSER_H
#define LOG_PARSER_H

#include "LogEntry.h"

#include <optional>
#include <string>

class LogParser {
public:
    std::optional<LogEntry> parseLine(const std::string& line, std::size_t lineNumber = 0) const;
};

#endif

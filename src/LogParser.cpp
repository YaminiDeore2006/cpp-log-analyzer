#include "LogParser.h"

#include "Timestamp.h"

#include <algorithm>
#include <cctype>
#include <regex>

std::string logLevelToString(LogLevel level) {
    switch (level) {
    case LogLevel::Debug:
        return "DEBUG";
    case LogLevel::Info:
        return "INFO";
    case LogLevel::Warning:
        return "WARNING";
    case LogLevel::Error:
        return "ERROR";
    }
    return "UNKNOWN";
}

std::optional<LogEntry> LogParser::parseLine(const std::string& line, std::size_t lineNumber) const {
    static const std::regex entryPattern(
        R"(^\[(\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2})\] \[(DEBUG|INFO|WARNING|ERROR)\] \[([^\]]+)\] (.*)$)");
    std::string normalizedLine = line;
    if (!normalizedLine.empty() && normalizedLine.back() == '\r') {
        normalizedLine.pop_back();
    }
    std::smatch matches;
    if (!std::regex_match(normalizedLine, matches, entryPattern)) {
        return std::nullopt;
    }

    const std::string timestamp = matches[1].str();
    const std::string module = matches[3].str();
    if (!isValidTimestamp(timestamp) ||
        std::all_of(module.begin(), module.end(), [](unsigned char character) {
            return std::isspace(character) != 0;
        })) {
        return std::nullopt;
    }

    LogLevel level;
    const std::string levelText = matches[2].str();
    if (levelText == "DEBUG") {
        level = LogLevel::Debug;
    } else if (levelText == "INFO") {
        level = LogLevel::Info;
    } else if (levelText == "WARNING") {
        level = LogLevel::Warning;
    } else {
        level = LogLevel::Error;
    }

    return LogEntry{
        timestamp,
        level,
        module,
        matches[4].str(),
        lineNumber
    };
}

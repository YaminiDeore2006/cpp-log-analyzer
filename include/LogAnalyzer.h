#ifndef LOG_ANALYZER_H
#define LOG_ANALYZER_H

#include "LogEntry.h"

#include <map>
#include <optional>
#include <string>
#include <vector>

struct AnalysisSummary {
    std::size_t totalEntries = 0;
    std::map<LogLevel, std::size_t> counts;
    double warningPercentage = 0.0;
    double errorPercentage = 0.0;
};

struct FrequentError {
    std::string message;
    std::size_t count = 0;
};

struct FilterCriteria {
    std::optional<LogLevel> level;
    std::string module;
    std::string keyword;
    std::string fromTimestamp;
    std::string toTimestamp;
};

class LogAnalyzer {
public:
    explicit LogAnalyzer(std::vector<LogEntry> entries);

    const std::vector<LogEntry>& entries() const noexcept;
    AnalysisSummary summarize() const;
    std::vector<FrequentError> frequentErrors(std::size_t limit = 5) const;
    std::map<std::string, std::size_t> errorsByModule() const;
    std::vector<LogEntry> filter(const FilterCriteria& criteria) const;

private:
    std::vector<LogEntry> entries_;
};

#endif

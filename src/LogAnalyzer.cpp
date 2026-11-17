#include "LogAnalyzer.h"

#include "Timestamp.h"

#include <algorithm>
#include <cctype>
#include <map>
#include <stdexcept>
#include <unordered_map>
#include <utility>

namespace {
std::string lowercase(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char character) {
        return static_cast<char>(std::tolower(character));
    });
    return value;
}
}

LogAnalyzer::LogAnalyzer(std::vector<LogEntry> entries) : entries_(std::move(entries)) {}

const std::vector<LogEntry>& LogAnalyzer::entries() const noexcept {
    return entries_;
}

AnalysisSummary LogAnalyzer::summarize() const {
    AnalysisSummary summary;
    summary.totalEntries = entries_.size();
    summary.counts = {
        {LogLevel::Debug, 0},
        {LogLevel::Info, 0},
        {LogLevel::Warning, 0},
        {LogLevel::Error, 0}
    };

    for (const LogEntry& entry : entries_) {
        ++summary.counts[entry.level];
    }

    if (summary.totalEntries != 0) {
        const double total = static_cast<double>(summary.totalEntries);
        summary.warningPercentage = static_cast<double>(summary.counts[LogLevel::Warning]) * 100.0 / total;
        summary.errorPercentage = static_cast<double>(summary.counts[LogLevel::Error]) * 100.0 / total;
    }
    return summary;
}

std::vector<FrequentError> LogAnalyzer::frequentErrors(std::size_t limit) const {
    std::unordered_map<std::string, std::size_t> counts;
    for (const LogEntry& entry : entries_) {
        if (entry.level == LogLevel::Error) {
            ++counts[entry.message];
        }
    }

    std::vector<FrequentError> errors;
    errors.reserve(counts.size());
    for (const auto& item : counts) {
        errors.push_back({item.first, item.second});
    }
    std::sort(errors.begin(), errors.end(), [](const FrequentError& left, const FrequentError& right) {
        if (left.count != right.count) {
            return left.count > right.count;
        }
        return left.message < right.message;
    });
    if (errors.size() > limit) {
        errors.resize(limit);
    }
    return errors;
}

std::map<std::string, std::size_t> LogAnalyzer::errorsByModule() const {
    std::map<std::string, std::size_t> counts;
    for (const LogEntry& entry : entries_) {
        if (entry.level == LogLevel::Error) {
            ++counts[entry.module];
        }
    }
    return counts;
}

std::vector<LogEntry> LogAnalyzer::filter(const FilterCriteria& criteria) const {
    if ((!criteria.fromTimestamp.empty() && !isValidTimestamp(criteria.fromTimestamp)) ||
        (!criteria.toTimestamp.empty() && !isValidTimestamp(criteria.toTimestamp))) {
        throw std::invalid_argument("Filter timestamps must use a valid YYYY-MM-DD HH:MM:SS value.");
    }
    if (!criteria.fromTimestamp.empty() && !criteria.toTimestamp.empty() &&
        criteria.fromTimestamp > criteria.toTimestamp) {
        throw std::invalid_argument("The start timestamp must not be later than the end timestamp.");
    }

    const std::string keyword = lowercase(criteria.keyword);
    std::vector<LogEntry> matching;
    for (const LogEntry& entry : entries_) {
        if (criteria.level && entry.level != *criteria.level) {
            continue;
        }
        if (!criteria.module.empty() && entry.module != criteria.module) {
            continue;
        }
        if (!criteria.fromTimestamp.empty() && entry.timestamp < criteria.fromTimestamp) {
            continue;
        }
        if (!criteria.toTimestamp.empty() && entry.timestamp > criteria.toTimestamp) {
            continue;
        }
        if (!keyword.empty() &&
            lowercase(entry.message).find(keyword) == std::string::npos &&
            lowercase(entry.module).find(keyword) == std::string::npos) {
            continue;
        }
        matching.push_back(entry);
    }
    return matching;
}

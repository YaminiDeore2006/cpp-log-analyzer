#include "ReportGenerator.h"

#include "FileManager.h"
#include "TextSafety.h"

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <locale>
#include <sstream>
#include <string>

namespace {
std::string csvEscape(const std::string& value) {
    const std::string safeValue = escapeControlCharacters(value);
    std::string escaped = "\"";
    const auto firstNonWhitespace = std::find_if_not(safeValue.begin(), safeValue.end(), [](unsigned char character) {
        return std::isspace(character) != 0;
    });
    if (firstNonWhitespace != safeValue.end() &&
        (*firstNonWhitespace == '=' || *firstNonWhitespace == '+' ||
         *firstNonWhitespace == '-' || *firstNonWhitespace == '@')) {
        escaped += '\'';
    }
    for (char character : safeValue) {
        if (character == '"') {
            escaped += "\"\"";
        } else {
            escaped += character;
        }
    }
    escaped += '"';
    return escaped;
}
}

void ReportGenerator::writeTextReport(const LogAnalyzer& analyzer, const std::filesystem::path& path) const {
    const AnalysisSummary summary = analyzer.summarize();
    const auto errors = analyzer.frequentErrors(5);
    const auto moduleCounts = analyzer.errorsByModule();

    std::ostringstream report;
    report << "Log Analysis Report\n"
           << "===================\n"
           << "Total valid entries: " << summary.totalEntries << '\n'
           << "DEBUG: " << summary.counts.at(LogLevel::Debug) << '\n'
           << "INFO: " << summary.counts.at(LogLevel::Info) << '\n'
           << "WARNING: " << summary.counts.at(LogLevel::Warning) << '\n'
           << "ERROR: " << summary.counts.at(LogLevel::Error) << '\n'
           << std::fixed << std::setprecision(2)
           << "Warning percentage: " << summary.warningPercentage << "%\n"
           << "Error percentage: " << summary.errorPercentage << "%\n\n"
           << "Top frequent errors\n"
           << "-------------------\n";
    if (errors.empty()) {
        report << "No errors found.\n";
    } else {
        for (std::size_t index = 0; index < errors.size(); ++index) {
            report << (index + 1) << ". " << escapeControlCharacters(errors[index].message)
                   << " (" << errors[index].count << ")\n";
        }
    }
    report << "\nErrors by module\n"
           << "----------------\n";
    if (moduleCounts.empty()) {
        report << "No errors found.\n";
    } else {
        for (const auto& item : moduleCounts) {
            report << escapeControlCharacters(item.first) << ": " << item.second << '\n';
        }
    }

    FileManager().writeText(path, report.str());
}

void ReportGenerator::writeCsvReport(const LogAnalyzer& analyzer, const std::filesystem::path& path) const {
    const AnalysisSummary summary = analyzer.summarize();
    std::ostringstream report;
    report.imbue(std::locale::classic());
    report << std::fixed << std::setprecision(2);
    report << "type,label,count,percentage\n";
    report << "summary,total," << summary.totalEntries << ','
           << (summary.totalEntries == 0 ? 0.0 : 100.0) << '\n';

    for (LogLevel level : {LogLevel::Debug, LogLevel::Info, LogLevel::Warning, LogLevel::Error}) {
        const std::size_t count = summary.counts.at(level);
        const double percentage = summary.totalEntries == 0
            ? 0.0
            : static_cast<double>(count) * 100.0 / static_cast<double>(summary.totalEntries);
        report << "level," << logLevelToString(level) << ',' << count << ',' << percentage << '\n';
    }
    for (const FrequentError& error : analyzer.frequentErrors(5)) {
        report << "frequent_error," << csvEscape(error.message) << ',' << error.count << ",\n";
    }
    for (const auto& item : analyzer.errorsByModule()) {
        report << "error_module," << csvEscape(item.first) << ',' << item.second << ",\n";
    }

    FileManager().writeText(path, report.str());
}

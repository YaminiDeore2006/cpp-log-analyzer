#include "FileManager.h"
#include "LogAnalyzer.h"
#include "LogParser.h"
#include "ReportGenerator.h"
#include "TextSafety.h"

#include <iomanip>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

namespace {
std::optional<LogLevel> parseLevel(const std::string& text) {
    if (text == "DEBUG") return LogLevel::Debug;
    if (text == "INFO") return LogLevel::Info;
    if (text == "WARNING") return LogLevel::Warning;
    if (text == "ERROR") return LogLevel::Error;
    return std::nullopt;
}

void printEntries(const std::vector<LogEntry>& entries) {
    if (entries.empty()) {
        std::cout << "No matching entries.\n";
        return;
    }
    for (const LogEntry& entry : entries) {
        std::cout << '[' << entry.timestamp << "] [" << logLevelToString(entry.level) << "] ["
                  << escapeControlCharacters(entry.module) << "] "
                  << escapeControlCharacters(entry.message) << '\n';
    }
}

void printSummary(const LogAnalyzer& analyzer) {
    const AnalysisSummary summary = analyzer.summarize();
    std::cout << "\nSummary (" << summary.totalEntries << " valid entries)\n"
              << "DEBUG: " << summary.counts.at(LogLevel::Debug) << "  "
              << "INFO: " << summary.counts.at(LogLevel::Info) << "  "
              << "WARNING: " << summary.counts.at(LogLevel::Warning) << "  "
              << "ERROR: " << summary.counts.at(LogLevel::Error) << '\n'
              << std::fixed << std::setprecision(2)
              << "Warning rate: " << summary.warningPercentage << "%  "
              << "Error rate: " << summary.errorPercentage << "%\n";
}
}

int main() {
    FileManager fileManager;
    LogParser parser;
    ReportGenerator reportGenerator;
    std::optional<LogAnalyzer> analyzer;
    std::size_t malformedLines = 0;

    while (true) {
        std::cout << "\n=== C++ Log Analyzer ===\n"
                  << "1. Load a log file\n"
                  << "2. Display summary\n"
                  << "3. Search logs by keyword\n"
                  << "4. Filter logs\n"
                  << "5. Show frequent errors\n"
                  << "6. Export reports\n"
                  << "0. Exit\n"
                  << "Choose an option: ";
        std::string choice;
        if (!std::getline(std::cin, choice)) {
            break;
        }

        try {
            if (choice == "0") {
                break;
            } else if (choice == "1") {
                std::cout << "Log file path (.log or .txt): ";
                std::string path;
                if (!std::getline(std::cin, path)) break;
                const std::vector<std::string> lines = fileManager.readLines(path);
                std::vector<LogEntry> entries;
                std::size_t invalidLines = 0;
                for (std::size_t index = 0; index < lines.size(); ++index) {
                    const auto entry = parser.parseLine(lines[index], index + 1);
                    if (entry) {
                        entries.push_back(*entry);
                    } else if (!lines[index].empty()) {
                        ++invalidLines;
                    }
                }
                analyzer.emplace(std::move(entries));
                malformedLines = invalidLines;
                std::cout << "Loaded " << analyzer->entries().size() << " valid entries; ignored "
                          << malformedLines << " malformed lines.\n";
            } else if (choice == "2") {
                if (!analyzer) {
                    std::cout << "Load a file first.\n";
                    continue;
                }
                printSummary(*analyzer);
                std::cout << "Malformed lines ignored: " << malformedLines << '\n';
            } else if (choice == "3") {
                if (!analyzer) {
                    std::cout << "Load a file first.\n";
                    continue;
                }
                std::cout << "Keyword: ";
                std::string keyword;
                if (!std::getline(std::cin, keyword)) break;
                FilterCriteria criteria;
                criteria.keyword = keyword;
                printEntries(analyzer->filter(criteria));
            } else if (choice == "4") {
                if (!analyzer) {
                    std::cout << "Load a file first.\n";
                    continue;
                }
                FilterCriteria criteria;
                std::cout << "Level (DEBUG/INFO/WARNING/ERROR, or blank): ";
                std::string level;
                if (!std::getline(std::cin, level)) break;
                if (!level.empty()) {
                    criteria.level = parseLevel(level);
                    if (!criteria.level) {
                        std::cout << "Invalid level.\n";
                        continue;
                    }
                }
                std::cout << "Module (exact, or blank): ";
                if (!std::getline(std::cin, criteria.module)) break;
                std::cout << "From timestamp (YYYY-MM-DD HH:MM:SS, or blank): ";
                if (!std::getline(std::cin, criteria.fromTimestamp)) break;
                std::cout << "To timestamp (YYYY-MM-DD HH:MM:SS, or blank): ";
                if (!std::getline(std::cin, criteria.toTimestamp)) break;
                printEntries(analyzer->filter(criteria));
            } else if (choice == "5") {
                if (!analyzer) {
                    std::cout << "Load a file first.\n";
                    continue;
                }
                const auto errors = analyzer->frequentErrors();
                if (errors.empty()) {
                    std::cout << "No errors found.\n";
                }
                for (std::size_t index = 0; index < errors.size(); ++index) {
                    std::cout << (index + 1) << ". " << escapeControlCharacters(errors[index].message)
                              << " (" << errors[index].count << ")\n";
                }
                std::cout << "Errors by module:\n";
                for (const auto& item : analyzer->errorsByModule()) {
                    std::cout << "  " << escapeControlCharacters(item.first) << ": " << item.second << '\n';
                }
            } else if (choice == "6") {
                if (!analyzer) {
                    std::cout << "Load a file first.\n";
                    continue;
                }
                reportGenerator.writeTextReport(*analyzer, "reports/analysis_report.txt");
                reportGenerator.writeCsvReport(*analyzer, "reports/analysis_report.csv");
                std::cout << "Wrote reports/analysis_report.txt and reports/analysis_report.csv\n";
            } else {
                std::cout << "Unknown option.\n";
            }
        } catch (const std::exception& exception) {
            std::cerr << "Error: " << exception.what() << '\n';
        }
    }
    std::cout << "Goodbye.\n";
    return 0;
}

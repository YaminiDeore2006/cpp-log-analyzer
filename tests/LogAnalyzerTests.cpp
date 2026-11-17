#include "FileManager.h"
#include "LogAnalyzer.h"
#include "LogParser.h"
#include "ReportGenerator.h"
#include "TextSafety.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>

namespace {
int checks = 0;

class TemporaryDirectory {
public:
    explicit TemporaryDirectory(const std::filesystem::path& path) : path_(path) {
        std::filesystem::create_directories(path_);
    }

    ~TemporaryDirectory() {
        std::error_code error;
        std::filesystem::remove_all(path_, error);
    }

    const std::filesystem::path& path() const noexcept {
        return path_;
    }

private:
    std::filesystem::path path_;
};

void check(bool condition, const std::string& message) {
    ++checks;
    if (!condition) {
        throw std::runtime_error(message);
    }
}

std::string readFile(const std::filesystem::path& path) {
    std::ifstream input(path);
    return std::string(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
}

void testParser() {
    LogParser parser;
    const auto parsed = parser.parseLine(
        "[2025-03-08 09:00:00] [ERROR] [Database] Connection failed", 7);
    check(parsed.has_value(), "valid log entry should parse");
    check(parsed->level == LogLevel::Error, "level should be parsed");
    check(parsed->module == "Database", "module should be parsed");
    check(parsed->message == "Connection failed", "message should be parsed");
    check(parsed->lineNumber == 7, "source line should be retained");
    check(!parser.parseLine("not a log entry").has_value(), "malformed entry should be rejected");
    check(!parser.parseLine("[2025-03-08 09:00:00] [TRACE] [API] unknown level").has_value(),
          "unsupported level should be rejected");
    check(!parser.parseLine("[2025-13-08 09:00:00] [INFO] [API] invalid month").has_value(),
          "out-of-range calendar dates should be rejected");
    check(!parser.parseLine("[2025-02-29 09:00:00] [INFO] [API] invalid leap day").has_value(),
          "non-leap-year February 29 should be rejected");
    check(parser.parseLine("[2024-02-29 09:00:00] [INFO] [API] valid leap day").has_value(),
          "valid leap-year February 29 should be accepted");
    check(parser.parseLine("[2025-03-08 09:00:00] [INFO] [API] CRLF input\r").has_value(),
          "CRLF-terminated entries should parse on all platforms");
    check(!parser.parseLine("[2025-03-08 09:00:00] [INFO] [API]missing separator").has_value(),
          "a missing message separator should be rejected");
    check(!parser.parseLine("[2025-03-08 09:00:00] [INFO] [   ] blank module").has_value(),
          "a whitespace-only module should be rejected");
}

std::vector<LogEntry> testEntries() {
    LogParser parser;
    const std::vector<std::string> lines = {
        "[2025-03-08 09:00:00] [INFO] [API] service ready",
        "[2025-03-08 09:01:00] [WARNING] [API] slow request",
        "[2025-03-08 09:02:00] [ERROR] [Database] connection failed",
        "[2025-03-08 09:03:00] [ERROR] [Database] connection failed",
        "[2025-03-08 09:04:00] [DEBUG] [Worker] retry scheduled"
    };
    std::vector<LogEntry> entries;
    for (std::size_t index = 0; index < lines.size(); ++index) {
        const auto entry = parser.parseLine(lines[index], index + 1);
        if (entry) {
            entries.push_back(*entry);
        }
    }
    return entries;
}

void testAnalysisAndFiltering() {
    const LogAnalyzer analyzer(testEntries());
    const AnalysisSummary summary = analyzer.summarize();
    check(summary.totalEntries == 5, "total count should match parsed entries");
    check(summary.counts.at(LogLevel::Info) == 1, "INFO count should be correct");
    check(summary.counts.at(LogLevel::Debug) == 1, "DEBUG count should be correct");
    check(summary.counts.at(LogLevel::Warning) == 1, "WARNING count should be correct");
    check(summary.counts.at(LogLevel::Error) == 2, "ERROR count should be correct");
    check(summary.errorPercentage == 40.0, "error percentage should be correct");
    check(summary.warningPercentage == 20.0, "warning percentage should be correct");

    const auto errors = analyzer.frequentErrors();
    check(errors.size() == 1 && errors.front().count == 2, "repeated errors should be grouped");
    check(errors.front().message == "connection failed", "error message should be preserved");
    check(analyzer.errorsByModule().at("Database") == 2, "errors should be grouped by module");

    FilterCriteria criteria;
    criteria.level = LogLevel::Error;
    criteria.module = "Database";
    criteria.fromTimestamp = "2025-03-08 09:03:00";
    check(analyzer.filter(criteria).size() == 1, "level, module, and timestamp filters should combine");
    criteria.fromTimestamp = "2025-03-08 09:02:00";
    criteria.toTimestamp = "2025-03-08 09:03:00";
    check(analyzer.filter(criteria).size() == 2, "timestamp filter endpoints should be inclusive");
    criteria.fromTimestamp = "2025-02-30 09:00:00";
    bool invalidTimestampRaised = false;
    try {
        analyzer.filter(criteria);
    } catch (const std::invalid_argument&) {
        invalidTimestampRaised = true;
    }
    check(invalidTimestampRaised, "invalid filter dates should report an error");
    criteria.fromTimestamp = "2025-03-08 09:04:00";
    criteria.toTimestamp = "2025-03-08 09:03:00";
    bool reversedRangeRaised = false;
    try {
        analyzer.filter(criteria);
    } catch (const std::invalid_argument&) {
        reversedRangeRaised = true;
    }
    check(reversedRangeRaised, "reversed timestamp ranges should report an error");
    criteria = FilterCriteria{};
    criteria.keyword = "SLOW";
    check(analyzer.filter(criteria).size() == 1, "keyword search should be case-insensitive");
}

void testFilesAndReports(const std::filesystem::path& directory) {
    FileManager files;
    const auto emptyPath = directory / "empty.txt";
    files.writeText(emptyPath, "");
    check(files.readLines(emptyPath).empty(), "empty files should load as zero lines");

    const auto crlfPath = directory / "crlf.log";
    {
        std::ofstream output(crlfPath, std::ios::binary);
        output << "[2025-03-08 09:00:00] [INFO] [API] CRLF entry\r\n\r\n";
    }
    const auto crlfLines = files.readLines(crlfPath);
    check(crlfLines.size() == 2 && !crlfLines[0].empty() && crlfLines[0].back() != '\r' &&
              crlfLines[1].empty(),
          "file loading should normalize CRLF lines and blank lines");

    bool missingRaised = false;
    try {
        files.readLines(directory / "missing.log");
    } catch (const std::runtime_error&) {
        missingRaised = true;
    }
    check(missingRaised, "missing files should report an error");

    bool unsupportedRaised = false;
    try {
        files.readLines(directory / "input.csv");
    } catch (const std::invalid_argument&) {
        unsupportedRaised = true;
    }
    check(unsupportedRaised, "unsupported input extensions should be rejected");

    const LogAnalyzer analyzer(testEntries());
    ReportGenerator reports;
    const auto textPath = directory / "nested" / "report.txt";
    const auto csvPath = directory / "report.csv";
    reports.writeTextReport(analyzer, textPath);
    reports.writeCsvReport(analyzer, csvPath);
    const std::string text = readFile(textPath);
    const std::string csv = readFile(csvPath);
    check(text.find("Total valid entries: 5") != std::string::npos, "text report should contain summary");
    check(text.find("connection failed (2)") != std::string::npos, "text report should contain frequent errors");
    check(csv.find("level,ERROR,2,40.00") != std::string::npos, "CSV should contain counts and percentages");
    check(csv.find("\"connection failed\"") != std::string::npos, "CSV should quote error messages");

    LogParser parser;
    const auto formulaEntry = parser.parseLine(
        "[2025-03-08 09:00:00] [ERROR] [API] =SUM(1, \"2\")");
    check(formulaEntry.has_value(), "formula-like error message should parse");
    const auto safeCsvPath = directory / "safe.csv";
    reports.writeCsvReport(LogAnalyzer({*formulaEntry}), safeCsvPath);
    check(readFile(safeCsvPath).find("\"'=SUM(1, \"\"2\"\")\"") != std::string::npos,
          "CSV should neutralize spreadsheet formulas and escape quotes");
    const auto controlEntry = parser.parseLine(
        "[2025-03-08 09:00:00] [ERROR] [API] \x1B[31mred");
    check(controlEntry.has_value(), "control characters in messages should not prevent parsing");
    const auto safeTextPath = directory / "safe.txt";
    reports.writeTextReport(LogAnalyzer({*controlEntry}), safeTextPath);
    check(readFile(safeTextPath).find("\\x1B[31mred") != std::string::npos,
          "text reports should render terminal controls as printable text");
    const auto safeControlCsvPath = directory / "safe_control.csv";
    reports.writeCsvReport(LogAnalyzer({*controlEntry}), safeControlCsvPath);
    check(readFile(safeControlCsvPath).find("\\x1B[31mred") != std::string::npos,
          "CSV reports should render control characters as printable text");

    const AnalysisSummary emptySummary = LogAnalyzer({}).summarize();
    check(emptySummary.totalEntries == 0 && emptySummary.errorPercentage == 0.0,
          "empty analysis should have zero totals and percentages");
    const auto emptyCsvPath = directory / "empty.csv";
    reports.writeCsvReport(LogAnalyzer({}), emptyCsvPath);
    check(readFile(emptyCsvPath).find("summary,total,0,0") != std::string::npos,
          "empty CSV report should show a zero total percentage");
}

void testSampleData(const std::filesystem::path& workspace) {
    FileManager files;
    LogParser parser;
    const auto lines = files.readLines(workspace / "data" / "sample.log");
    std::size_t valid = 0;
    std::size_t malformed = 0;
    for (const std::string& line : lines) {
        if (parser.parseLine(line)) {
            ++valid;
        } else if (!line.empty()) {
            ++malformed;
        }
    }
    check(valid >= 15, "sample dataset should provide a realistic set of valid entries");
    check(malformed >= 2, "sample dataset should contain malformed entries");
}
}

int main() {
    try {
        const auto unique = std::chrono::steady_clock::now().time_since_epoch().count();
        const TemporaryDirectory directory(
            std::filesystem::temp_directory_path() / ("cpp_log_analyzer_tests_" + std::to_string(unique)));

        testParser();
        testAnalysisAndFiltering();
        testFilesAndReports(directory.path());
        testSampleData(std::filesystem::current_path());

        std::cout << "All " << checks << " checks passed.\n";
        return 0;
    } catch (const std::exception& exception) {
        std::cerr << "Test failure after " << checks << " checks: " << exception.what() << '\n';
        return 1;
    }
}

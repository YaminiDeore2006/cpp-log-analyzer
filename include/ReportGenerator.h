#ifndef REPORT_GENERATOR_H
#define REPORT_GENERATOR_H

#include "LogAnalyzer.h"

#include <filesystem>

class ReportGenerator {
public:
    void writeTextReport(const LogAnalyzer& analyzer, const std::filesystem::path& path) const;
    void writeCsvReport(const LogAnalyzer& analyzer, const std::filesystem::path& path) const;
};

#endif

#include "FileManager.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <stdexcept>

namespace {
std::string lowercase(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char character) {
        return static_cast<char>(std::tolower(character));
    });
    return value;
}
}

std::vector<std::string> FileManager::readLines(const std::filesystem::path& path) const {
    const std::string extension = lowercase(path.extension().string());
    if (extension != ".log" && extension != ".txt") {
        throw std::invalid_argument("Unsupported input file type; expected .log or .txt.");
    }

    std::ifstream input(path);
    if (!input.is_open()) {
        throw std::runtime_error("Unable to open input file: " + path.string());
    }

    std::vector<std::string> lines;
    std::string line;
    while (std::getline(input, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        lines.push_back(line);
    }
    if (input.bad()) {
        throw std::runtime_error("Error while reading input file: " + path.string());
    }
    return lines;
}

void FileManager::writeText(const std::filesystem::path& path, const std::string& content) const {
    const std::filesystem::path parent = path.parent_path();
    if (!parent.empty() && !std::filesystem::exists(parent)) {
        std::filesystem::create_directories(parent);
    }

    std::ofstream output(path);
    if (!output.is_open()) {
        throw std::runtime_error("Unable to open output file: " + path.string());
    }
    output << content;
    output.flush();
    if (!output) {
        throw std::runtime_error("Error while writing output file: " + path.string());
    }
    output.close();
    if (!output) {
        throw std::runtime_error("Error while closing output file: " + path.string());
    }
}

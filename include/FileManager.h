#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H

#include <filesystem>
#include <string>
#include <vector>

class FileManager {
public:
    std::vector<std::string> readLines(const std::filesystem::path& path) const;
    void writeText(const std::filesystem::path& path, const std::string& content) const;
};

#endif

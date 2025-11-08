#pragma once

#include "boost/filesystem/path.hpp"
#include "logging.h"
#include <fstream>
#include <string>
#include <vector>

auto split(std::string string, const std::string& delimiter) -> std::vector<std::string> {
    std::vector<std::string> tokens;
    size_t pos = 0;
    std::string token;
    while ((pos = string.find(delimiter)) != std::string::npos) {
        token = string.substr(0, pos);
        tokens.push_back(token);
        string.erase(0, pos + delimiter.length());
    }
    tokens.push_back(string);

    return tokens;
}

auto read_file(const boost::filesystem::path& path) -> std::string {
    std::ifstream file(path);

    if (file.is_open()) {
        return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    }

    push_log(warning) << "Tried to open a file: " << path << " but it could not be read." << end_log;
    return {};
}

auto read_file_lines(const boost::filesystem::path& path) -> std::vector<std::string> {
    std::ifstream file(path);

    if (file.is_open()) {
        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        file.close();

        return split(content, "\n");
    }

    push_log(warning) << "Tried to open a file: " << path << " but it could not be read." << end_log;
    return {};
}

auto open(const boost::filesystem::path& path) -> std::ofstream {
    std::ofstream out(path);
    if (!out.is_open()) {
        push_log(warning) << "Could not open path " << path << " even though it exists." << end_log;
    }
    return out;
}
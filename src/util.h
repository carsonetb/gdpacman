#pragma once

#include "boost/filesystem/path.hpp"
#include "boost/json/object.hpp"
#include "logging.h"
#include <fstream>
#include <string>
#include <vector>

#define VERSION_MAJOR 1
#define VERSION_MINOR 1
#define VERSION_PATCH 0

struct Source {
    bool invalid = false;

    std::string source;
    std::string branch;
    std::string path;

    bool has_branch = false;

    [[nodiscard]] auto to_json() const -> boost::json::object {
        boost::json::object out;
        out["source"] = source;
        out["path"] = path;
        if (has_branch) { out["branch"] = branch; }
        return out;
    }
};

struct DepsFile {
    bool invalid = false;

    std::vector<Source> sources;

    bool has_addon_folder_path = false;
    std::string addon_folder_path;

    [[nodiscard]] auto to_json() const -> boost::json::object {
        boost::json::object out;
        boost::json::array deps;
        for (const auto& source : sources) {
            deps.push_back(source.to_json());
        }
        out["deps"] = deps;
        if (has_addon_folder_path) {
            out["this"] = addon_folder_path;
        }
        return out;
    }
};

auto create_source_from_json(const boost::json::value& source) -> Source {
    Source out;
    if (!source.is_object()) {
        push_log(error) << "Invalid .deps format: Each array item must be an object." << end_log;
        out.invalid = true;
        return out;
    }
    auto as_object = source.as_object();
    if (!as_object.contains("source") || !as_object.contains("path")) {
        push_log(error) << "Invalid .deps format: Each source must contain at least a source and path property." << end_log;
        out.invalid = true;
        return out;
    }
    out.source = as_object.at("source").as_string();
    out.path = as_object.at("path").as_string();
    if (as_object.contains("branch")) {
        out.has_branch = true;
        out.branch = as_object.at("branch").as_string();
    }
    return out;
}

auto create_deps_from_json(const boost::json::value& json) -> DepsFile {
    DepsFile out;

    if (!json.is_object()) {
        push_log(error) << "Deps file must be an object." << end_log;
        out.invalid = true;
        return out;
    }
    auto as_object = json.as_object();
    if (!as_object.contains("deps") || !as_object.at("deps").is_array()) {
        push_log(error) << "Deps file must have an entry named deps of type array." << end_log;
        out.invalid = true;
        return out;
    }
    auto deps = as_object.at("deps").as_array();
    for (const auto& source : deps) {
        Source compiled = create_source_from_json(source);
        if (compiled.invalid) {
            out.invalid = true;
            return out;
        }
        out.sources.push_back(compiled);
    }

    if (as_object.contains("this")) {
        out.has_addon_folder_path = true;
        out.addon_folder_path = as_object.at("this").as_string();
    }

    return out;
}

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

auto get_name_from_source(const std::string& source) -> std::string {
    if (source.back() == '/') {
        auto split_path = split(source, "/");
        return split_path.at(split_path.size() - 2);
    }
    return split(source, "/").back();
}

auto read_file(const boost::filesystem::path& path) -> std::string {
    std::ifstream file(path);

    if (file.is_open()) {
        return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    }

    push_log(warning) << "Tried to open a file: " << path << " but it could not be read." << end_log;
    return "";
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
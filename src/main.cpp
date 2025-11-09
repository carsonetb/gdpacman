#include <boost/filesystem/config.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/exception.hpp>
#include <boost/filesystem/directory.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/file_status.hpp>
#include <boost/range/algorithm/find.hpp>
#include "boost/program_options/value_semantic.hpp"
#include "boost/program_options/options_description.hpp"
#include "boost/program_options/parsers.hpp"
#include "boost/program_options/variables_map.hpp"
#include <git2.h>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "logging.h"
#include "util.h"

namespace po = boost::program_options;
namespace filesystem = boost::filesystem;

#define log_help_and_return(text) push_log(warning) << text << end_log; push_log(info) << desc << end_log; return 1;

auto init_sources(const filesystem::path& project_path, std::vector<std::string> sources, git_repository *repo) -> int {
    auto clone_home = filesystem::path("/tmp/gdpacman");

    if (filesystem::exists(clone_home)) {
        push_log(debug) << "Clearing temporary sources to prevent conflicts." << end_log;
        filesystem::remove_all(clone_home);
    }

    while (!sources.empty()) {
        const std::string source = sources.back();
        sources.pop_back();
        auto name = split(source, "/").back();
        auto clone_path = clone_home / name;

        push_log(debug) << "Cloning repo at " << source << end_log;
        
        git_repository *source_repo = nullptr;
        int clone_error = git_clone(&source_repo, source.c_str(), clone_path.c_str(), nullptr);
        if (clone_error < 0) {
            push_log(error) << "Git error when cloning repo: " << git_error_last()->message << end_log;
            return -1;
        }

        auto deps_path = clone_path / ".deps";
        std::string addon_folder_name;
        if (filesystem::exists(deps_path) && filesystem::is_regular_file(deps_path)) {
            push_log(debug) << "This addon has a dependencies file, using." << end_log;
            auto lines = read_file_lines(deps_path);
            addon_folder_name = lines[0]; // The first line of a .deps should be the folder name of the addon to use.

            if (addon_folder_name.find(' ') != std::string::npos) {
                push_log(error) << "Invalid .deps format. The first line should be the name of the addon folder name (like addons/addon_folder_name would be addon_folder_name)" << end_log;
                continue;
            }

            for (const std::string& line : lines) {
                if (line == addon_folder_name || line.empty()) {
                    continue;
                }
                push_log(debug) << "Addon at path " << source << " has dependency " << line << ", adding." << end_log;
                auto name = split(line, " ").front();
                sources.push_back(name);
            }
        }

        push_log(debug) << "Copying contents of source addons folder to project addons." << end_log;

        auto source_addons = clone_path / "addons";
        auto project_addons = project_path / "addons";

        if (!filesystem::exists(source_addons)) {
            push_log(error) << "Cloned repo doesn't have an addons folder.";
            git_repository_free(source_repo);
            return 1;
        }
        
        bool ignore_this = false;
        for (const auto& entry : filesystem::directory_iterator(source_addons)) {
            if (filesystem::is_directory(entry) && (addon_folder_name.empty() || entry.path().filename().string() == addon_folder_name)) {
                auto addon_folder = project_addons / name;
                if (filesystem::exists(addon_folder)) {
                    push_log(prompt) << "Folder for this addon already exists, should it be deleted? (N/y) " << end_log_no_newline;
                    std::string response;
                    std::getline(std::cin, response);
                    if (response == "y") {
                        filesystem::remove_all(addon_folder);
                    }
                    else {
                        ignore_this = true;
                        break;
                    }
                }
                filesystem::create_directories(project_addons / name);
                filesystem::copy(entry.path(), project_addons / name);
                break;
            }
        }

        if (ignore_this) {
            push_log(debug) << "Addon was not fully initialized, but that's okay because it already exists." << end_log;
            continue;
        }

        if (filesystem::exists(deps_path)) {
            push_log(debug) << "Moving .deps to the addon's folder." << end_log;
            filesystem::copy(deps_path, project_addons / name / ".deps");
        }

        push_log(info) << "Finished initializing addon " << name << end_log;

        git_repository_free(source_repo);
    }

    push_log(info) << "All addons added successfully!" << end_log;

    return 0;
}

auto init_deps(const filesystem::path& project_path, std::vector<std::string> sources) -> void {
    auto deps_path = project_path / ".deps";

    if (filesystem::exists(deps_path)) {
        std::ifstream ideps_file(deps_path);

        if (ideps_file.is_open()) {
            std::string content((std::istreambuf_iterator<char>(ideps_file)), std::istreambuf_iterator<char>());
            ideps_file.close();

            auto lines = split(content, "\n");
            for (const std::string& line : lines) {
                if (line.empty()) {
                    continue;
                }
                auto path = split(line, " ").front();
                if (boost::range::find(sources, path) == sources.end()) {
                    sources.push_back(path);
                }
            }
        }
        else {
            push_log(warning) << "Can't read deps file (bust it exists), assuming it is empty." << end_log;
        }
    }

    push_log(debug) << "Writing sources to .deps file" << end_log;
    std::ofstream deps_file(deps_path);

    if (deps_file.is_open()) {
        for (const auto& source : sources) {
            deps_file << source << " " << split(source, "/").back() << "\n";
        }
        deps_file.close();
    }
    else {
        push_log(error) << "Cannot write to .deps file" << end_log;
    }
}

auto remove_addons(const filesystem::path& project_path, const std::vector<std::string>& names) -> void {
    auto addons_folder = project_path / "addons";

    if (!filesystem::exists(addons_folder)) {
        push_log(error) << "There is no addons folder here, so probably no addons to remove!" << end_log;
        return;
    }

    for (const std::string& name : names) {
        auto this_folder = addons_folder / name;

        if (!filesystem::exists(this_folder)) {
            push_log(warning) << "No addon with name " << name << end_log;
            continue;
        }

        if (filesystem::exists(this_folder / ".deps")) {
            auto lines = read_file_lines(this_folder / ".deps");

            if (!lines.empty()) {
                push_log(debug) << "Addon " << name << " has dependencies that you might want to remove, listing:" << end_log;
                for (const auto& line : lines) {
                    if (line.empty()) {
                        continue;
                    }
                    push_log(debug) << line << end_log;
                }
            }
        }

        filesystem::remove_all(this_folder);

        push_log(info) << "Finished removing addon " << name << end_log;
    }

    push_log(debug) << "Reloading dependencies" << end_log;
    auto current_deps = read_file_lines(project_path / ".deps");
    auto deps_file = open(project_path / ".deps");
    for (const std::string& line : current_deps) {
        if (line.empty()) {
            continue;
        }
        auto dep_name = split(line, " ").back();
        bool removed = false;
        for (const std::string& removed_name : names) {
            if (dep_name == removed_name) {
                removed = true;
                break;
            }
        }
        if (!removed) {
            deps_file << line << "\n";
        }
    }
    deps_file.close();
}

auto main(int argc, char **argv) -> int {
    po::options_description desc("gdpacman allowed options");
    desc.add_options()
        ("help,h", "produce (this) help message")
        ("remove,r", po::value<std::vector<std::string>>()->multitoken(), "names of addon(s) to remove.")
        ("url,u", po::value<std::vector<std::string>>()->multitoken(), "git url(s) of the package(s)")
        ("project,p", po::value<std::string>(), "the path to the project where the package will be installed")
        ("register", po::value<std::string>(), "for addon developers, the name of the addon folder");
    
    po::variables_map varmap;
    po::store(po::parse_command_line(argc, argv, desc), varmap);
    po::notify(varmap);

    if (varmap.count("help") > 0) {
        push_log(info) << desc << end_log;
        return 1;
    }

    filesystem::path project_path = ".";
    if (varmap.count("project") > 0) {
        project_path = varmap["project"].as<std::string>();
    }
    else {
        push_log(debug) << "Using the current path as the project path." << end_log;
    }

    if (varmap.count("register") > 0) {
        auto deps_lines = read_file_lines(project_path / ".deps");
        auto deps_file = open(project_path / ".deps");
        auto name = varmap["register"].as<std::string>();

        if (deps_lines.front().find(' ') == std::string::npos) {
            push_log(warning) << "Overriding old registered path which was " << deps_lines.front() << end_log;
            deps_lines.erase(deps_lines.begin());
        }

        deps_file << name << '\n';
        for (const std::string& line : deps_lines) {
            deps_file << line << "\n";
        }
        deps_file.close();
        push_log(info) << "Registered " << name << ". When your addon is installed, only this and your .deps file will be moved to the user's project." << end_log;
    }
    else if (varmap.count("remove") > 0) {
        remove_addons(project_path, varmap["remove"].as<std::vector<std::string>>());
    }
    else if (varmap.count("url") > 0) {
        auto sources = varmap["url"].as<std::vector<std::string>>();

        if (!filesystem::exists(project_path / "project.godot")) {
            push_log(error) << "No project.godot file exists at the project path." << end_log;
            return 1;
        }

        git_libgit2_init();

        git_repository *repo = nullptr;
        int open_error = git_repository_open(&repo, project_path.string().c_str());
        if (open_error < 0) {
            push_log(error) << "Git error trying to open repository: " << git_error_last()->message << end_log;
            return 1;
        }

        init_deps(project_path, sources);

        if (init_sources(project_path, sources, repo) == 1) {
            push_log(fatal) << "Failed to initialize sources! (Your .deps file may be corrupted, check it!)" << end_log;
            git_repository_free(repo);
            git_libgit2_shutdown();
            return 1;
        }

        git_repository_free(repo);

        git_libgit2_shutdown();
    }
    else {
        log_help_and_return("Nothing to do!");
    }

    return 0;
}
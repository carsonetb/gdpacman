#include <boost/filesystem/config.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/exception.hpp>
#include <boost/filesystem/directory.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/file_status.hpp>
#include <boost/range/algorithm/find.hpp>
#include "boost/json/array.hpp"
#include "boost/json/parse.hpp"
#include "boost/json/serialize.hpp"
#include "boost/program_options/options_description.hpp"
#include "boost/program_options/parsers.hpp"
#include "boost/program_options/variables_map.hpp"
#include <git2.h>
#include <fstream>
#include <git2/global.h>
#include <string>
#include <vector>

#include "git2/clone.h"
#include "logging.h"
#include "util.h"

namespace po = boost::program_options;
namespace filesystem = boost::filesystem;

#define log_help_and_return(text) push_log(warning) << text << end_log; push_log(info) << desc << end_log; return 1;

auto init_sources(const filesystem::path& project_path, std::vector<std::string> sources) -> int {
    auto clone_home = filesystem::path("/tmp/gdpacman");

    if (filesystem::exists(clone_home)) {
        push_log(debug) << "Clearing temporary sources to prevent conflicts." << end_log;
        filesystem::remove_all(clone_home);
    }

    while (!sources.empty()) {
        std::string source = sources.back();
        auto split_colon = split(source, "::");
        std::string branch;
        if (split_colon.size() > 1) {
            source = split_colon.front();
            branch = split_colon.back();
        }
        sources.pop_back();
        auto name = split(source, "/").back();
        auto clone_path = clone_home / name;

        push_log(debug) << "Cloning repo at " << source << (branch.empty() ? ", default branch" : ", branch: " + branch) << end_log;
        
        git_repository *source_repo = nullptr;
        git_clone_options clone_options = GIT_CLONE_OPTIONS_INIT;
        if (!branch.empty()) {
            clone_options.checkout_branch = branch.c_str();
        }
        int clone_error = git_clone(&source_repo, source.c_str(), clone_path.string().c_str(), &clone_options);
        if (clone_error < 0) {
            push_log(error) << "Git error when cloning repo: " << git_error_last()->message << end_log;
            return -1;
        }

        auto deps_path = clone_path / ".deps";
        std::string addon_folder_name;
        if (filesystem::exists(deps_path) && filesystem::is_regular_file(deps_path)) {
            push_log(debug) << "This addon has a dependencies file, using." << end_log;
            auto file = create_deps_from_json(boost::json::parse(read_file(deps_path)));
            if (file.invalid || !file.has_addon_folder_path) {
                push_log(error) << "The addon with the deps file just mentioned is invalid! Things are about to break. Also, contact the maintainer of the addon." << end_log;
                return -1;
            }
            addon_folder_name = split(file.addon_folder_path, "/").back();

            for (const Source& source : file.sources) {
                push_log(debug) << "Addon at path " << source.path << " has dependency " << source.source << ", adding." << end_log;
                std::string source_txt = source.source;
                if (source.has_branch) {
                    source_txt += "::" + source.branch;
                }
                sources.push_back(source.source);
            }
        }

        auto source_addons = clone_path / "addons";
        auto project_addons = project_path / "addons";

        push_log(debug) << "Copying contents of source addons folder to " << project_addons / name << end_log;

        if (!filesystem::exists(source_addons)) {
            push_log(error) << "Cloned repo doesn't have an addons folder." << end_log;
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
            push_log(debug) << "Moving .deps to the addons folder." << end_log;
            filesystem::copy(deps_path, project_addons / name / ".deps");
        }

        push_log(info) << "Finished initializing addon " << name << end_log;

        git_repository_free(source_repo);
    }

    push_log(info) << "All addons added successfully!" << end_log;

    return 0;
}

auto init_deps(const filesystem::path& project_path, const std::vector<std::string>& sources) -> int {
    auto deps_path = project_path / ".deps";

    if (!filesystem::exists(deps_path)) {
        push_log(error) << ".deps file doesn't exist in this project. Use gdpacman --init to create it." << end_log;
        return 1;
    }

    std::vector<Source> write_sources;

    boost::json::array out_deps_array;
    boost::json::value current_deps_val = boost::json::parse(read_file(deps_path));
    if (!current_deps_val.is_object()) {
        push_log(error) << "Invalid .deps format: It must be an object." << end_log;
        return 1;
    }
    boost::json::object deps_file = current_deps_val.as_object();
    if (!deps_file.contains("deps") || !deps_file.at("deps").is_array()) {
        push_log(error) << "Invalid .deps format: It must have a deps value that is an array." << end_log;
        return 1;
    }
    boost::json::array in_deps_array = deps_file.at("deps").as_array();
    for (const auto& source : in_deps_array) {
        write_sources.push_back(create_source_from_json(source));
    }
    bool has_addon_folder_path = false;
    std::string addon_folder_path;
    if (deps_file.contains("this")) {
        has_addon_folder_path = true;
        addon_folder_path = deps_file.at("this").as_string();
    }

    for (const auto& input : sources) {
        std::string source = input;
        auto split_colon = split(source, "::");
        std::string branch;
        if (split_colon.size() > 1) {
            source = split_colon.front();
            branch = split_colon.back();
        }
        Source this_source;
        this_source.source = source;
        this_source.path = (filesystem::path("addons") / get_name_from_source(this_source.source)).string();
        if (!branch.empty()) {
            this_source.has_branch = true;
            this_source.branch = branch;
        }
        bool already_acquired = false;
        for (const auto& source : write_sources) {
            if (source.source == this_source.source) {
                already_acquired = true;
                break;
            }
        }
        if (already_acquired) { continue; }
        write_sources.push_back(this_source);
    }

    for (const auto& source : write_sources) {
        out_deps_array.push_back(source.to_json());
    }

    boost::json::object out_deps;
    out_deps["deps"] = out_deps_array;
    if (has_addon_folder_path) {
        out_deps["this"] = addon_folder_path;
    }

    push_log(debug) << "Writing sources to .deps file" << end_log;
    std::ofstream write_file(deps_path);

    if (write_file.is_open()) {
        write_file << boost::json::serialize(out_deps);
        write_file.close();
    }
    else {
        push_log(error) << "Cannot write to .deps file" << end_log;
    }

    return 0;
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
                auto file = create_deps_from_json(boost::json::parse(read_file(this_folder / ".deps")));
                if (file.invalid) {
                    push_log(warning) << "Nevermind, the .deps file is invalid" << end_log;
                }
                else {
                    for (const Source& source : file.sources) {
                        push_log(debug) << source.path << end_log;
                    }
                }
            }
        }

        filesystem::remove_all(this_folder);

        push_log(info) << "Finished removing addon " << name << end_log;
    }

    push_log(debug) << "Reloading dependencies" << end_log;
    auto current_deps = create_deps_from_json(boost::json::parse(read_file(project_path / ".deps")));
    DepsFile out_deps;
    out_deps.has_addon_folder_path = current_deps.has_addon_folder_path;
    out_deps.addon_folder_path = current_deps.addon_folder_path;
    if (current_deps.invalid) {
        push_log(error) << "Reload dependencies failed, .deps file is probably corrupted." << end_log;
        return;
    }
    for (const Source& source : current_deps.sources) {
        bool removed = false;
        auto name = split(source.path, "/").back();
        for (const std::string& removed_name : names) {
            if (name == removed_name) {
                removed = true;
                break;
            }
        }
        if (!removed) {
            out_deps.sources.push_back(source);
        }
    }
    auto deps_file = open(project_path / ".deps");
    deps_file << boost::json::serialize(out_deps.to_json());
    deps_file.close();
}

auto main(int argc, char **argv) -> int {
    po::options_description desc("gdpacman allowed options");
    desc.add_options()
        ("help,h", "produce (this) help message")
        ("version,v", "get the gdpacman version")
        ("project,p", po::value<std::string>(), "the path to the project where the package will be installed, if not set the current working directory will be used")
        ("init", "initialize the .deps file, this can be called once in a new project.")
        ("remove,r", po::value<std::vector<std::string>>()->multitoken(), "names of addon(s) to remove, their folders will be deleted and they will be removed from the .deps file")
        ("url,u", po::value<std::vector<std::string>>()->multitoken(), "git url(s) of the package(s) to be installed in the addons directory, and added to the .deps file")
        ("update", "updates all the addons, beware that all folders will be deleted and recloned")
        ("register", po::value<std::string>(), "for addon developers, the name of the addon folder");
    
    po::variables_map varmap;
    po::store(po::parse_command_line(argc, argv, desc), varmap);
    po::notify(varmap);

    if (varmap.count("help") > 0) {
        push_log(info) << desc << end_log;
        return 0;
    }

    if (varmap.count("version") > 0) {
        push_log(info) << "gdpacman version " << VERSION_MAJOR << "." << VERSION_MINOR << "." << VERSION_PATCH << end_log;
        return 0;
    }

    filesystem::path project_path = ".";
    if (varmap.count("project") > 0) {
        project_path = varmap["project"].as<std::string>();
    }
    else {
        push_log(debug) << "Using the current path as the project path." << end_log;
    }

    if (varmap.count("update") > 0) {
        git_libgit2_init();
        push_log(info) << "Reloading all sources. If you want the new version to be added, say yes to the prompt!" << end_log;
        if (!filesystem::exists(project_path / ".deps")) {
            push_log(error) << ".deps file doesn't exist, use gdpacman --init to initialize it." << end_log;
            return 1;
        }
        std::vector<std::string> sources;
        DepsFile file = create_deps_from_json(boost::json::parse(read_file(project_path / ".deps")));
        if (file.invalid) {
            push_log(error) << ".deps file is invalid." << end_log;
            return 1;
        }
        for (const Source& source : file.sources) {
            if (source.has_branch) {
                sources.push_back(source.source + "::" + source.branch);
            }
            else {
                sources.push_back(source.source);
            }
        }
        init_sources(project_path, sources);
        git_libgit2_shutdown();
    }
    else if (varmap.count("init") > 0) {
        auto canonical_path = filesystem::canonical(project_path);
        if (filesystem::exists(project_path / ".deps")) {
            push_log(error) << ".deps file already exists at path " << canonical_path << ". Exiting to save your dependencies!" << end_log;
            return 1;
        }
        auto deps_file = open(project_path / ".deps");
        deps_file << boost::json::serialize(DepsFile().to_json());
        deps_file.close();
        push_log(info) << "Sucessfully initialized a project at path " << canonical_path << end_log;
    }
    else if (varmap.count("register") > 0) {
        if (!filesystem::exists(project_path / ".deps")) {
            push_log(error) << ".deps file doesn't exist, use gdpacman --init to initialize it." << end_log;
            return 1;
        }
        DepsFile deps = create_deps_from_json(boost::json::parse(read_file(project_path / ".deps")));
        if (deps.invalid) {
            push_log(error) << ".deps file is invalid." << end_log;
            return 1;
        }
        auto name = varmap["register"].as<std::string>();

        if (deps.has_addon_folder_path) {
            push_log(warning) << "Overriding old registered path which was " << deps.addon_folder_path << end_log;
        }

        deps.has_addon_folder_path = true;
        deps.addon_folder_path = (filesystem::path("addons") / name).string();

        auto deps_file = open(project_path / ".deps");
        deps_file << boost::json::serialize(deps.to_json()) << "\n";
        deps_file.close();
        push_log(info) << "Registered " << deps.addon_folder_path << ". When your addon is installed, only this and your .deps file will be moved to the user's project." << end_log;
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

        int init_error = init_deps(project_path, sources);
        if (init_error > 0) {
            push_log(error) << "Error initializing dependencies. Exiting" << end_log;
            return 1;
        }

        if (init_sources(project_path, sources) == 1) {
            push_log(fatal) << "Failed to initialize sources! (Your .deps file may be corrupted, check it!)" << end_log;
            git_libgit2_shutdown();
            return 1;
        }

        git_libgit2_shutdown();
    }
    else {
        log_help_and_return("Nothing to do!");
    }

    return 0;
}
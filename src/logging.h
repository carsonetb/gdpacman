#pragma once

#include <cassert>
#include <cstdint>
#include <iostream>
#include <ostream>
#include <string>
#include <unordered_map>

#define end_log "\033[0m\n"
#define end_log_no_newline "\033[0m"

enum severity_level : uint8_t {
    debug,
    info,
    warning,
    error,
    fatal
};

static const std::unordered_map<uint8_t, std::string> severity_level_to_color = {
    {debug, "\033[38;5;33mDEBUG: "},
    {info, "\033[38;5;112mINFO: "},
    {warning, "\033[4;38;5;220mWARNING: "},
    {error, "\033[1;31mERROR: "},
    {fatal, "\033[1;38;5;232;48;5;160m: FATAL: "},
};

auto push_log(uint8_t severity) -> std::ostream& {
    assert(severity_level_to_color.count(severity));
    return std::cout << severity_level_to_color.at(severity);
}
#pragma once

#include "termcolor/termcolor.hpp"
#include <cassert>
#include <cstdint>
#include <iostream>
#include <ostream>
#include <string>
#include <unordered_map>

#define end_log termcolor::reset << "\n"
#define end_log_no_newline termcolor::reset

enum severity_level : uint8_t {
    debug,
    info,
    warning,
    error,
    fatal,
    prompt,
};

static const std::unordered_map<uint8_t, std::string> severity_level_to_color = {
    {debug, "\033[38;5;33mDEBUG: "},
    {info, "\033[38;5;112mINFO: "},
    {warning, "\033[4;38;5;220mWARNING: "},
    {error, "\033[1;31mERROR: "},
    {fatal, "\033[1;38;5;232;48;5;160mFATAL: "},
    {prompt, "\033[38;5;164mPROMPT: "},
};

auto push_log(uint8_t severity) -> std::ostream& {
    assert(severity_level_to_color.count(severity));
    switch (severity) {
        case debug:
            return std::cout << termcolor::color<33> << "DEBUG: ";
        case info:
            return std::cout << termcolor::color<112> << "INFO: ";
        case warning:
            return std::cout << termcolor::underline << termcolor::color<220> << "WARNING: ";
        case error:
            return std::cout << termcolor::bold << termcolor::red << "ERROR: ";
        case fatal:
            return std::cout << termcolor::bold << termcolor::on_color<160> << "FATAL: ";
        case prompt:
            return std::cout << termcolor::color<164> << "PROMPT: ";
        default:
            return std::cout;
    }
    return std::cout;
}
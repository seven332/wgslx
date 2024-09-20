#pragma once

#include <src/tint/lang/wgsl/program/program.h>

#include <string>
#include <string_view>
#include <unordered_map>

namespace wgslx::minifier {

struct Options {
    bool rename_identifiers = true;
    bool remove_unreachable_statements = true;
    bool remove_useless_functions = true;
};

struct Result {
    tint::Program program;
    std::unordered_map<std::string, std::string> remappings;
    std::string failure_message;
    bool failed = false;
};

Result Minify(std::string_view data, const Options& options);

}  // namespace wgslx::minifier

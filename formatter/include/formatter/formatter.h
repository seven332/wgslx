#pragma once

#include <src/tint/lang/wgsl/program/program.h>

#include <string>

namespace wgslx::formatter {

struct Options {};

struct Result {
    std::string wgsl;
    std::string failure_message;
    bool failed = false;
};

Result Format(const tint::Program& program, const Options& options);

}  // namespace wgslx::formatter

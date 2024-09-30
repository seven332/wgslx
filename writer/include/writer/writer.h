#pragma once

#include <src/tint/lang/wgsl/program/program.h>

#include <string>

namespace wgslx::writer {

struct Options {
    bool precise_float = false;
    bool use_type_alias = true;
    bool ignore_literal_suffix = true;
};

struct Result {
    std::string wgsl;
    std::string failure_message;
    bool failed = false;
};

Result Write(const tint::Program& program, const Options& options);

}  // namespace wgslx::writer

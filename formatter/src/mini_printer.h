#pragma once

#include <src/tint/lang/wgsl/program/program.h>
#include <src/tint/utils/generator/text_generator.h>

#include <sstream>
#include <string>

namespace wgslx::formatter {

class MiniPrinter {
 public:
    explicit MiniPrinter(const tint::Program* program) : program_(program) {}

    bool Generate();

    std::string Result();

 private:
    const tint::Program* program_;
    std::stringstream ss_;

    void EmitEnables();
    void EmitRequires();
};

}  // namespace wgslx::formatter

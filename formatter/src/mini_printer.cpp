#include "mini_printer.h"

#include <src/tint/lang/wgsl/ast/enable.h>
#include <src/tint/lang/wgsl/ast/module.h>

#include <algorithm>
#include <range/v3/range/conversion.hpp>
#include <range/v3/view/for_each.hpp>
#include <vector>

namespace wgslx::formatter {

bool MiniPrinter::Generate() {
    EmitEnables();
    return true;
}

void MiniPrinter::EmitEnables() {
    const auto& enables = program_->AST().Enables();
    std::vector<tint::wgsl::Extension> extensions;
    for (const auto* enable : enables) {
        for (const auto* extension : enable->extensions) {
            extensions.emplace_back(extension->name);
        }
    }
    std::sort(extensions.begin(), extensions.end(), [](tint::wgsl::Extension a, tint::wgsl::Extension b) {
        return a < b;
    });
    auto last = std::unique(extensions.begin(), extensions.end());
    extensions.erase(last, extensions.end());

    if (!extensions.empty()) {
        ss_ << "enable ";
        for (auto iter = extensions.begin(); iter != extensions.end(); ++iter) {
            if (iter != extensions.begin()) {
                ss_ << ",";
            }
            ss_ << *iter;
        }
        ss_ << ";";
    }
}

std::string MiniPrinter::Result() {
    return std::move(ss_).str();
}

}  // namespace wgslx::formatter

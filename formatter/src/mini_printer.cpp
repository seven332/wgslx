#include "mini_printer.h"

#include <src/tint/lang/wgsl/ast/enable.h>
#include <src/tint/lang/wgsl/ast/module.h>
#include <src/tint/lang/wgsl/features/language_feature.h>

#include <algorithm>
#include <range/v3/range/conversion.hpp>
#include <range/v3/view/for_each.hpp>
#include <vector>

namespace wgslx::formatter {

bool MiniPrinter::Generate() {
    EmitEnables();
    EmitRequires();
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

void MiniPrinter::EmitRequires() {
    const auto& requireList = program_->AST().Requires();
    std::vector<tint::wgsl::LanguageFeature> features;
    for (const auto* require : requireList) {
        for (auto feature : require->features) {
            features.emplace_back(feature);
        }
    }
    std::sort(features.begin(), features.end(), [](tint::wgsl::LanguageFeature a, tint::wgsl::LanguageFeature b) {
        return a < b;
    });
    auto last = std::unique(features.begin(), features.end());
    features.erase(last, features.end());

    if (!features.empty()) {
        ss_ << "requires ";
        for (auto iter = features.begin(); iter != features.end(); ++iter) {
            if (iter != features.begin()) {
                ss_ << ",";
            }
            ss_ << tint::wgsl::ToString(*iter);
        }
        ss_ << ";";
    }
}

std::string MiniPrinter::Result() {
    return std::move(ss_).str();
}

}  // namespace wgslx::formatter

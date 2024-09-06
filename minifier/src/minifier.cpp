#include "minifier/minifier.h"

#include <src/tint/lang/wgsl/reader/reader.h>
#include <src/tint/lang/wgsl/writer/writer.h>
#include <src/tint/utils/diagnostic/diagnostic.h>

#include <range/v3/all.hpp>

namespace wgslx::minifier {

static constexpr const auto* DefaultPath = "temp.wgsl";

static Result GenerateError(const tint::diag::List& diagnostics) {
    auto message = diagnostics | ranges::views::filter([](const tint::diag::Diagnostic& d) {
                       return d.severity == tint::diag::Severity::Error;
                   }) |
                   ranges::views::transform([](const tint::diag::Diagnostic& d) { return d.message.Plain(); }) |
                   ranges::views::join('\n') | ranges::to<std::string>();
    return {
        .failed = true,
        .failureMessage = std::move(message),
    };
}

Result Minify(std::string_view data, const Options& options) {
    tint::Source::File file(DefaultPath, data);
    auto program = tint::wgsl::reader::Parse(
        &file,
        {
            .allowed_features = tint::wgsl::AllowedFeatures::Everything(),
            .mode = tint::wgsl::ValidationMode::kFull,
        }
    );
    if (program.Diagnostics().ContainsErrors()) {
        return GenerateError(program.Diagnostics());
    }

    auto result = tint::wgsl::writer::Generate(program, {});
    if (result != tint::Success) {
        return GenerateError(result.Failure().reason);
    }

    return {
        .wgsl = result->wgsl,
    };
}

}  // namespace wgslx::minifier

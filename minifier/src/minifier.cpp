#include "minifier/minifier.h"

#include "tint/tint.h"

namespace wgslx::minifier {

static constexpr const auto* DefaultPath = "temp.wgsl";

Result Minify(std::string_view data, const Options& options) {
    tint::Source::File file(DefaultPath, data);
    auto program = tint::wgsl::reader::Parse(
        &file,
        {
            .allowed_features = tint::wgsl::AllowedFeatures::Everything(),
            .mode = tint::wgsl::ValidationMode::kFull,
        }
    );

    auto result = tint::wgsl::writer::Generate(program, {});
    if (result != tint::Success) {
        return {.failed = true};
    }

    return {
        .wgsl = result->wgsl,
        .failed = false,
    };
}

}  // namespace wgslx::minifier

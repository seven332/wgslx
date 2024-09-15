#include "formatter/formatter.h"

#include <gmock/gmock.h>
#include <src/tint/lang/wgsl/common/allowed_features.h>
#include <src/tint/lang/wgsl/common/validation_mode.h>
#include <src/tint/lang/wgsl/program/program.h>
#include <src/tint/lang/wgsl/reader/reader.h>
#include <src/tint/utils/diagnostic/diagnostic.h>

#include <range/v3/algorithm/fold_left.hpp>
#include <range/v3/range/primitives.hpp>
#include <range/v3/view/filter.hpp>
#include <range/v3/view/transform.hpp>
#include <string>
#include <unordered_map>

#include "gmock/gmock.h"

namespace wgslx::minifier {

static tint::Program Parse(const char* code) {
    tint::Source::File file("", code);
    auto program = tint::wgsl::reader::Parse(
        &file,
        {
            .allowed_features = tint::wgsl::AllowedFeatures::Everything(),
            .mode = tint::wgsl::ValidationMode::kFull,
        }
    );
    auto errors = program.Diagnostics() | ranges::views::filter([](const tint::diag::Diagnostic& diag) {
                      return diag.severity == tint::diag::Severity::Error;
                  });
    EXPECT_EQ(errors.begin(), errors.end());
    return program;
}

TEST(formatter, enable) {
    auto program = Parse("enable f16, dual_source_blending; enable clip_distances, clip_distances;");
    auto result = formatter::Format(program, {});
    EXPECT_EQ(result.wgsl, "enable clip_distances,dual_source_blending,f16;");
}

TEST(formatter, require) {
    auto program = Parse(
        "requires readonly_and_readwrite_storage_textures, packed_4x8_integer_dot_product; requires pointer_composite_access, pointer_composite_access;"
    );
    auto result = formatter::Format(program, {});
    EXPECT_EQ(
        result.wgsl,
        "requires packed_4x8_integer_dot_product,pointer_composite_access,readonly_and_readwrite_storage_textures;"
    );
}

TEST(formatter, diagnostic) {
    auto program = Parse("diagnostic(off, test.a); diagnostic(error, test.b);");
    auto result = formatter::Format(program, {});
    EXPECT_EQ(result.wgsl, "diagnostic(off,test.a);diagnostic(error,test.b);");
}

}  // namespace wgslx::minifier

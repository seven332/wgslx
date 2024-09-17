#include "writer/writer.h"

#include <gmock/gmock.h>
#include <src/tint/lang/wgsl/common/allowed_features.h>
#include <src/tint/lang/wgsl/common/validation_mode.h>
#include <src/tint/lang/wgsl/program/program.h>
#include <src/tint/lang/wgsl/reader/reader.h>
#include <src/tint/lang/wgsl/writer/writer.h>
#include <src/tint/utils/diagnostic/diagnostic.h>

#include <iostream>
#include <random>
#include <range/v3/algorithm/fold_left.hpp>
#include <range/v3/range/primitives.hpp>
#include <range/v3/view/filter.hpp>
#include <range/v3/view/transform.hpp>
#include <string>
#include <unordered_map>

#include "gmock/gmock.h"

namespace wgslx::writer {

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

    for (const auto& e : errors) {
        std::cout << e.message.Plain() << std::endl;
    }

    EXPECT_EQ(errors.begin(), errors.end());
    return program;
}

TEST(writer, enable) {
    auto program = Parse("enable f16, dual_source_blending; enable clip_distances, clip_distances;");
    auto result = Write(program, {});
    EXPECT_EQ(result.wgsl, "enable clip_distances,dual_source_blending,f16;");
}

TEST(writer, require) {
    auto program = Parse(
        "requires readonly_and_readwrite_storage_textures, packed_4x8_integer_dot_product; requires pointer_composite_access, pointer_composite_access;"
    );
    auto result = Write(program, {});
    EXPECT_EQ(
        result.wgsl,
        "requires packed_4x8_integer_dot_product,pointer_composite_access,readonly_and_readwrite_storage_textures;"
    );
}

TEST(writer, diagnostic) {
    auto program = Parse("diagnostic(off, test.a); diagnostic(error, test.b);");
    auto result = Write(program, {});
    EXPECT_EQ(result.wgsl, "diagnostic(off,test.a);diagnostic(error,test.b);");
}

class RandomExpression {
 public:
    std::string next() {
        switch (gen_() % 2) {
        case 0:
            return nextBool();
        case 1:
        default:
            return nextU32();
        }
    }

    std::string nextBool() {
        if (gen_() % 2 == 0) {
            return "false";
        } else {
            switch (gen_() % 9) {
            case 0:
                return "(" + nextBool() + "&&" + nextBool() + ")";
            case 1:
                return "(" + nextBool() + "||" + nextBool() + ")";
            case 2:
                return "(" + nextU32() + "==" + nextU32() + ")";
            case 3:
                return "(" + nextU32() + "!=" + nextU32() + ")";
            case 4:
                return "(" + nextU32() + "<" + nextU32() + ")";
            case 5:
                return "(" + nextU32() + ">" + nextU32() + ")";
            case 6:
                return "(" + nextU32() + "<=" + nextU32() + ")";
            case 7:
                return "(" + nextU32() + ">=" + nextU32() + ")";
            case 8:
            default:
                return "(!" + nextBool() + ")";
            }
        }
    }

    std::string nextU32() {
        if (gen_() % 2 == 0) {
            return std::to_string(gen_() % 1024) + "u";
        } else {
            switch (gen_() % 7) {
            case 0:
                return "(" + nextU32() + "&" + nextU32() + ")";
            case 1:
                return "(" + nextU32() + "|" + nextU32() + ")";
            case 2:
                return "(" + nextU32() + "^" + nextU32() + ")";
            case 3:
                return "(" + nextU32() + "+" + nextU32() + ")";
            case 4:
                return "(" + nextU32() + "-" + nextU32() + ")";
            case 5:
                return "(" + nextU32() + "*" + nextU32() + ")";
            case 6:
            default:
                return "(~" + nextU32() + ")";
            }
        }
    }

 private:
    std::minstd_rand gen_ {0};  // NOLINT
};

TEST(writer, expression) {
    RandomExpression random;
    for (auto i = 0; i < 100; ++i) {
        auto code = "const a = " + random.next() + ";";
        auto program1 = Parse(code.c_str());
        auto result = Write(program1, {});
        auto program2 = Parse(result.wgsl.c_str());
        auto r1 = tint::wgsl::writer::Generate(program1, {});
        auto r2 = tint::wgsl::writer::Generate(program2, {});
        EXPECT_EQ(r1->wgsl, r2->wgsl);
    }
}

TEST(writer, attributes) {
    auto program = Parse("@group(1) @binding(0) var a: sampler;");
    auto result = Write(program, {});
    EXPECT_EQ(result.wgsl, "@group(1)@binding(0)var a:sampler;");
}

TEST(writer, struct) {
    auto program = Parse("struct A {u: f32, v: f32, w: vec2<f32>, @size(16) x: f32}");
    auto result = Write(program, {});
    EXPECT_EQ(result.wgsl, "struct A{u:f32,v:f32,w:vec2<f32>,@size(16)x:f32}");
}

}  // namespace wgslx::writer

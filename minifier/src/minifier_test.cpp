#include "minifier/minifier.h"

#include <gmock/gmock.h>
#include <src/tint/lang/wgsl/program/program.h>
#include <src/tint/lang/wgsl/writer/writer.h>

#include <string>

namespace wgslx::minifier {

static std::string Write(const tint::Program& program) {
    auto result = tint::wgsl::writer::Generate(program, {});
    return result->wgsl;
}

TEST(minifier, Rename) {
    auto result = Minify(
        R"(
fn average(a: f32, b: f32) -> f32 {
    return (a + b) / 2;
}

@vertex fn vs1() -> @builtin(position) vec4f {
    return vec4f(average(0, 1));
}
)",
        {}
    );
    EXPECT_FALSE(result.failed);
    EXPECT_EQ(
        Write(result.program),
        "fn c(d : f32, e : f32) -> f32 {\n  return ((d + e) / 2.0f);\n}\n"
        "\n@vertex\nfn f() -> @builtin(position) vec4f {\n  return vec4f(c(0.0f, 1.0f));\n}\n"
    );
    EXPECT_THAT(result.remappings, testing::UnorderedElementsAre(testing::Pair("vs1", "f")));
}

TEST(minifier, MinifyFailed) {
    auto result = Minify(
        R"(
fn average(a: f32, b: f3) -> f32 {
    return (a + b) / 2
}
)",
        {}
    );
    EXPECT_TRUE(result.failed);
    EXPECT_EQ(result.failure_message, "expected ';' for return statement");
}

TEST(minifier, RenameSkipKeywords) {
    std::string input = "fn f1() -> i32 {";
    for (auto i = 0; i < 4000; ++i) {
        input += "let a";
        input += std::to_string(i);
        input += " = 0;";
    }
    input += "return 0;}";

    auto result = Minify(
        input,
        {
            .rename_identifiers = true,
            .remove_unreachable_statements = false,
            .remove_useless = false,
            .fold_constants = false,
        }
    );
    EXPECT_FALSE(result.failed);
    EXPECT_NE(Write(result.program).find("let aq = 0;"), std::string::npos);
    EXPECT_EQ(Write(result.program).find("let ar = 0;"), std::string::npos);
    EXPECT_EQ(Write(result.program).find("let as = 0;"), std::string::npos);
    EXPECT_NE(Write(result.program).find("let at = 0;"), std::string::npos);
}

TEST(minifier, RenameSkipSwizzle) {
    auto result = Minify(
        R"(
fn getA(color: vec4f) -> f32 {
    return color.a;
}

@vertex fn vs1() -> @builtin(position) vec4f {
    return vec4f(getA(vec4f(1)), 1, 1, 1);
}
)",
        {}
    );
    EXPECT_FALSE(result.failed);
    EXPECT_EQ(
        Write(result.program),
        "fn c(d : vec4f) -> f32 {\n  return d.a;\n}\n\n@vertex\nfn e() -> @builtin(position) vec4f {\n  return vec4f(c(vec4<f32>(1.0f)), 1.0f, 1.0f, 1.0f);\n}\n"
    );
    EXPECT_THAT(result.remappings, testing::UnorderedElementsAre(testing::Pair("vs1", "e")));
}

TEST(minifier, RemoveUnreachable) {
    auto result = Minify(
        R"(
@vertex fn vs1() -> @builtin(position) vec4f {
    return vec4f(2);
    return vec4f(1);
    return vec4f(0);
}
)",
        {}
    );
    EXPECT_FALSE(result.failed);
    EXPECT_EQ(Write(result.program), "@vertex\nfn c() -> @builtin(position) vec4f {\n  return vec4<f32>(2.0f);\n}\n");
    EXPECT_THAT(result.remappings, testing::UnorderedElementsAre(testing::Pair("vs1", "c")));
}

TEST(minifier, RemoveUselessFunctions) {
    auto result = Minify(
        R"(
fn average(a: f32, b: f32) -> f32 {
    return (a + b) / 2;
}

@vertex fn vs1() -> @builtin(position) vec4f {
    return vec4f(1);
}
)",
        {}
    );
    EXPECT_FALSE(result.failed);
    EXPECT_EQ(Write(result.program), "@vertex\nfn c() -> @builtin(position) vec4f {\n  return vec4<f32>(1.0f);\n}\n");
    EXPECT_THAT(result.remappings, testing::UnorderedElementsAre(testing::Pair("vs1", "c")));
}

TEST(minifier, RemoveUselessConst) {
    auto result = Minify(
        R"(
const i = 2;
const j = 2;

@vertex fn vs1() -> @builtin(position) vec4f {
    return vec4f(1) / i;
}
)",
        {}
    );
    EXPECT_FALSE(result.failed);
    EXPECT_EQ(Write(result.program), "@vertex\nfn c() -> @builtin(position) vec4f {\n  return vec4<f32>(0.5f);\n}\n");
    EXPECT_THAT(result.remappings, testing::UnorderedElementsAre(testing::Pair("vs1", "c")));
}

TEST(minifier, RemoveUselessVar) {
    auto result = Minify(
        R"(
@vertex fn vs1() -> @builtin(position) vec4f {
    let i = 2.0;
    const j = 2.0;
    let k = 2.0;
    return vec4f(1) / i / j;
}
)",
        {}
    );
    EXPECT_FALSE(result.failed);
    EXPECT_EQ(
        Write(result.program),
        "@vertex\nfn c() -> @builtin(position) vec4f {\n  let d = 2.0f;\n  return ((vec4<f32>(1.0f) / d) / 2.0f);\n}\n"
    );
    EXPECT_THAT(result.remappings, testing::UnorderedElementsAre(testing::Pair("vs1", "c")));
}

}  // namespace wgslx::minifier

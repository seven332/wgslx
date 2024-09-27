#include "minifier/minifier.h"

#include <gmock/gmock.h>
#include <src/tint/lang/wgsl/program/program.h>
#include <src/tint/lang/wgsl/writer/writer.h>

#include <string>
#include <unordered_map>

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
        "fn a(b : f32, c : f32) -> f32 {\n  return ((b + c) / 2);\n}\n"
        "\n@vertex\nfn d() -> @builtin(position) vec4f {\n  return vec4f(a(0, 1));\n}\n"
    );
    EXPECT_THAT(result.remappings, testing::UnorderedElementsAre(testing::Pair("vs1", "d")));
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
        }
    );
    EXPECT_FALSE(result.failed);
    EXPECT_NE(Write(result.program).find("let ar = 0;"), std::string::npos);
    EXPECT_EQ(Write(result.program).find("let as = 0;"), std::string::npos);
    EXPECT_NE(Write(result.program).find("let at = 0;"), std::string::npos);
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
    EXPECT_EQ(Write(result.program), "@vertex\nfn a() -> @builtin(position) vec4f {\n  return vec4f(2);\n}\n");
    EXPECT_THAT(result.remappings, testing::UnorderedElementsAre(testing::Pair("vs1", "a")));
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
    EXPECT_EQ(Write(result.program), "@vertex\nfn a() -> @builtin(position) vec4f {\n  return vec4f(1);\n}\n");
    EXPECT_THAT(result.remappings, testing::UnorderedElementsAre(testing::Pair("vs1", "a")));
}

}  // namespace wgslx::minifier

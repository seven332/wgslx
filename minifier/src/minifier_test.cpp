#include "minifier/minifier.h"

#include <gtest/gtest.h>

namespace wgslx::minifier {

TEST(minifier, Minify) {
    auto result = Minify(
        R"(
fn average(a: f32, b: f32) -> f32 {
    return (a + b) / 2;
}
)",
        {}
    );
    EXPECT_FALSE(result.failed);
    EXPECT_EQ(result.wgsl, "fn average(a : f32, b : f32) -> f32 {\n  return ((a + b) / 2);\n}\n");
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
    EXPECT_EQ(result.failureMessage, "expected ';' for return statement");
}

}  // namespace wgslx::minifier

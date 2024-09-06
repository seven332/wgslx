#pragma once

#include <string>
#include <string_view>

namespace wgslx::minifier {

struct Options {};

struct Result {
    std::string wgsl;
    std::string failureMessage;
    bool failed = false;
};

Result Minify(std::string_view data, const Options& options);

}  // namespace wgslx::minifier

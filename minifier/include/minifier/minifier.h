#pragma once

#include <string>
#include <string_view>
#include <unordered_map>

namespace wgslx::minifier {

struct Options {};

struct Result {
    std::string wgsl;
    std::unordered_map<std::string, std::string> remappings;
    std::string failure_message;
    bool failed = false;
};

Result Minify(std::string_view data, const Options& options);

}  // namespace wgslx::minifier

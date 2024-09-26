#pragma once

#include <functional>
#include <string>
#include <string_view>

namespace wgslx::parser {

struct Options {
    std::function<std::string(std::string_view)> reader;
};

struct Result {
    std::string wgsl;
};

Result Parse(std::string_view data, const Options& options);

}  // namespace wgslx::parser

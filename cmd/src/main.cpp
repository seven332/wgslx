#include <src/tint/utils/cli/cli.h>
#include <src/tint/utils/containers/transform.h>

#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>

#include "minifier/minifier.h"
#include "writer/writer.h"

struct Options {
    std::string input;
};

static bool ParseArgs(tint::VectorRef<std::string_view> arguments, Options* opts) {
    tint::cli::OptionSet options;

    auto& help = options.Add<tint::cli::BoolOption>("help", "Show usage", tint::cli::ShortName {"h"});

    auto show_usage = [&] {
        std::cout << R"(Usage: wgslx <input-file>

Options:
)";
        options.ShowHelp(std::cout);
    };

    auto result = options.Parse(arguments);
    if (result != tint::Success) {
        std::cerr << result.Failure() << "\n";
        show_usage();
        return false;
    }
    if (help.value.value_or(false)) {
        show_usage();
        return false;
    }

    auto files = result.Get();
    if (files.IsEmpty()) {
        show_usage();
        return false;
    }
    if (files.Length() > 1) {
        std::cerr << "More than one input file specified: " << tint::Join(tint::Transform(files, tint::Quote), ", ")
                  << "\n";
        return false;
    }
    if (files.Length() == 1) {
        opts->input = files[0];
    }

    return true;
}

int main(int argc, char* argv[]) {
    tint::Vector<std::string_view, 8> arguments;
    for (int i = 1; i < argc; i++) {
        std::string_view arg(argv[i]);
        if (!arg.empty()) {
            arguments.Push(argv[i]);
        }
    }

    Options options;
    if (!ParseArgs(arguments, &options)) {
        return 1;
    }

    std::fstream file(options.input, std::ios_base::in | std::ios_base::binary);
    std::stringstream stream;
    stream << file.rdbuf();
    auto content = std::move(stream).str();

    auto minifier_res = wgslx::minifier::Minify(content, {});
    if (minifier_res.failed) {
        std::cerr << minifier_res.failure_message << "\n";
        return 1;
    }

    auto writer_res = wgslx::writer::Write(minifier_res.program, {});
    if (writer_res.failed) {
        std::cerr << writer_res.failure_message << "\n";
        return 1;
    }

    nlohmann::json j;
    j["wgsl"] = writer_res.wgsl;
    j["remappings"] = minifier_res.remappings;
    std::cout << j.dump() << "\n";

    return 0;
}

#include "minifier/minifier.h"

#include <src/tint/lang/wgsl/ast/transform/fold_constants.h>
#include <src/tint/lang/wgsl/ast/transform/manager.h>
#include <src/tint/lang/wgsl/ast/transform/remove_unreachable_statements.h>
#include <src/tint/lang/wgsl/reader/reader.h>
#include <src/tint/lang/wgsl/writer/writer.h>
#include <src/tint/utils/diagnostic/diagnostic.h>

#include <range/v3/range/conversion.hpp>
#include <range/v3/view/filter.hpp>
#include <range/v3/view/join.hpp>
#include <range/v3/view/transform.hpp>

#include "remove_useless.h"
#include "rename_identifiers.h"

namespace wgslx::minifier {

static constexpr const auto* DefaultPath = "temp.wgsl";

static Result GenerateError(const tint::diag::List& diagnostics) {
    auto message = diagnostics | ranges::views::filter([](const tint::diag::Diagnostic& d) {
                       return d.severity == tint::diag::Severity::Error;
                   }) |
                   ranges::views::transform([](const tint::diag::Diagnostic& d) { return d.message.Plain(); }) |
                   ranges::views::join('\n') | ranges::to<std::string>();
    return {
        .failure_message = std::move(message),
        .failed = true,
    };
}

Result Minify(std::string_view data, const Options& options) {
    // Parse
    tint::Source::File file(DefaultPath, data);
    auto input = tint::wgsl::reader::Parse(
        &file,
        {
            .allowed_features = tint::wgsl::AllowedFeatures::Everything(),
            .mode = tint::wgsl::ValidationMode::kFull,
        }
    );
    if (input.Diagnostics().ContainsErrors()) {
        return GenerateError(input.Diagnostics());
    }

    // Transform
    tint::ast::transform::Manager transform_manager;
    tint::ast::transform::DataMap in_data;
    tint::ast::transform::DataMap out_data;
    if (options.remove_unreachable_statements) {
        transform_manager.Add<tint::ast::transform::RemoveUnreachableStatements>();
    }
    if (options.fold_constants) {
        transform_manager.Add<tint::ast::transform::FoldConstants>();
    }
    if (options.remove_useless) {
        transform_manager.Add<RemoveUseless>();
    }
    if (options.rename_identifiers) {
        transform_manager.Add<RenameIdentifiers>();
    }

    auto output = transform_manager.Run(input, in_data, out_data);

    std::unordered_map<std::string, std::string> remappings;
    if (options.rename_identifiers) {
        remappings = std::move(out_data.Get<RenameIdentifiers::Data>()->remappings);
    }

    return {
        .program = std::move(output),
        .remappings = std::move(remappings),
    };
}

}  // namespace wgslx::minifier

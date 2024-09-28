#pragma once

#include <string>
#include <unordered_map>
#include <utility>

#include "src/tint/lang/wgsl/ast/transform/transform.h"

namespace wgslx::minifier {

class RenameIdentifiers final : public tint::Castable<RenameIdentifiers, tint::ast::transform::Transform> {
 public:
    using Remappings = std::unordered_map<std::string, std::string>;

    struct Data final : public Castable<Data, tint::ast::transform::Data> {
        explicit Data(Remappings&& r) : remappings(std::move(r)) {}
        Remappings remappings;
    };

    ApplyResult Apply(
        const tint::Program& program,
        const tint::ast::transform::DataMap& inputs,
        tint::ast::transform::DataMap& outputs
    ) const override;
};

}  // namespace wgslx::minifier

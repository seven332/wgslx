#pragma once

#include <string>
#include <unordered_map>

#include "src/tint/lang/wgsl/ast/transform/transform.h"

namespace wgslx::minifier {

class Renamer final : public tint::Castable<Renamer, tint::ast::transform::Transform> {
 public:
    using Remappings = std::unordered_map<std::string, std::string>;

    struct Data final : public Castable<Data, tint::ast::transform::Data> {
        explicit Data(Remappings&& r) : remappings(r) {}
        Remappings remappings;
    };

    ApplyResult Apply(
        const tint::Program& program,
        const tint::ast::transform::DataMap& inputs,
        tint::ast::transform::DataMap& outputs
    ) const override;
};

}  // namespace wgslx::minifier

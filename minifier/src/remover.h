#pragma once

#include "src/tint/lang/wgsl/ast/transform/transform.h"

namespace wgslx::minifier {

class Remover final : public tint::Castable<Remover, tint::ast::transform::Transform> {
 public:
    ApplyResult Apply(
        const tint::Program& program,
        const tint::ast::transform::DataMap& inputs,
        tint::ast::transform::DataMap& outputs
    ) const override;
};

}  // namespace wgslx::minifier

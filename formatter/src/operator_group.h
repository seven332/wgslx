#pragma once

// https://www.w3.org/TR/WGSL/#operator-precedence-associativity

#include <src/tint/lang/core/binary_op.h>

#include <cstdint>

namespace wgslx::formatter {

enum class OperatorGroup : std::uint8_t {
    None,
    Primary,
    Unary,
    Multiplicative,
    Additive,
    Shift,
    Relational,
    BinaryAND,
    BinaryXOR,
    BinaryOR,
    ShortCircuitAND,
    ShortCircuitOR,
};

enum class OperatorPosition : std::uint8_t {
    Left,
    Right,
};

OperatorGroup toOperatorGroup(tint::core::BinaryOp op);

bool isParenthesisRequired(OperatorGroup self, OperatorPosition position, OperatorGroup parent);

}  // namespace wgslx::formatter

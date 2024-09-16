#include "operator_group.h"

#include <cstdint>

namespace wgslx::formatter {

OperatorGroup toOperatorGroup(tint::core::BinaryOp op) {
    switch (op) {
    case tint::core::BinaryOp::kAnd:
        return OperatorGroup::BinaryAND;
    case tint::core::BinaryOp::kOr:
        return OperatorGroup::BinaryOR;
    case tint::core::BinaryOp::kXor:
        return OperatorGroup::BinaryXOR;
    case tint::core::BinaryOp::kLogicalAnd:
        return OperatorGroup::ShortCircuitAND;
    case tint::core::BinaryOp::kLogicalOr:
        return OperatorGroup::ShortCircuitOR;
    case tint::core::BinaryOp::kEqual:
    case tint::core::BinaryOp::kNotEqual:
    case tint::core::BinaryOp::kLessThan:
    case tint::core::BinaryOp::kGreaterThan:
    case tint::core::BinaryOp::kLessThanEqual:
    case tint::core::BinaryOp::kGreaterThanEqual:
        return OperatorGroup::Relational;
    case tint::core::BinaryOp::kShiftLeft:
    case tint::core::BinaryOp::kShiftRight:
        return OperatorGroup::Shift;
    case tint::core::BinaryOp::kAdd:
    case tint::core::BinaryOp::kSubtract:
        return OperatorGroup::Additive;
    case tint::core::BinaryOp::kMultiply:
    case tint::core::BinaryOp::kDivide:
    case tint::core::BinaryOp::kModulo:
        return OperatorGroup::Multiplicative;
    }
}

static bool isGroup1(OperatorGroup group) {
    return group == OperatorGroup::ShortCircuitOR || group == OperatorGroup::ShortCircuitAND ||
           group == OperatorGroup::BinaryOR || group == OperatorGroup::BinaryAND || group == OperatorGroup::BinaryXOR;
}

static bool isGroup2(OperatorGroup group) {
    return group == OperatorGroup::Shift || group == OperatorGroup::Relational;
}

bool isParenthesisRequired(OperatorGroup self, OperatorPosition position, OperatorGroup parent) {
    if (parent == OperatorGroup::None) {
        return false;
    }

    if (isGroup1(self) && isGroup1(parent) && self != parent) {
        return true;
    }

    if (isGroup1(self) && self == parent) {
        return true;
    }

    switch (position) {
    case OperatorPosition::Left:
        return parent == OperatorGroup::Unary ? static_cast<std::uint8_t>(self) >= static_cast<std::uint8_t>(parent)
                                              : static_cast<std::uint8_t>(self) > static_cast<std::uint8_t>(parent);
    case OperatorPosition::Right:
        return parent == OperatorGroup::Unary ? static_cast<std::uint8_t>(self) > static_cast<std::uint8_t>(parent)
                                              : static_cast<std::uint8_t>(self) >= static_cast<std::uint8_t>(parent);
    }
}

}  // namespace wgslx::formatter

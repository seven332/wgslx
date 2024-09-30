#include "operator_group.h"

namespace wgslx::writer {

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

static bool isNotMixable(OperatorGroup group) {
    return group == OperatorGroup::ShortCircuitOR || group == OperatorGroup::ShortCircuitAND ||
           group == OperatorGroup::BinaryOR || group == OperatorGroup::BinaryAND || group == OperatorGroup::BinaryXOR;
}

static bool isAssociativityRequiresParentheses(OperatorGroup group) {
    return group == OperatorGroup::Shift || group == OperatorGroup::Relational;
}

static bool isBindingUnary(OperatorGroup group) {
    return group == OperatorGroup::Shift || group == OperatorGroup::BinaryAND || group == OperatorGroup::BinaryXOR ||
           group == OperatorGroup::BinaryOR;
}

static bool isBindingRelational(OperatorGroup group) {
    return group == OperatorGroup::ShortCircuitAND || group == OperatorGroup::ShortCircuitOR;
}

bool isParenthesisRequired(OperatorGroup self, OperatorPosition position, OperatorGroup parent) {
    if (parent == OperatorGroup::None) {
        return false;
    }

    if (isNotMixable(self) && isNotMixable(parent) && self != parent) {
        return true;
    }

    if (isAssociativityRequiresParentheses(self) && self == parent) {
        return true;
    }

    if (isBindingUnary(parent) && self > OperatorGroup::Unary) {
        return true;
    }

    if (isBindingRelational(parent) && self > OperatorGroup::Relational) {
        return true;
    }

    switch (position) {
    case OperatorPosition::Left:
        return parent == OperatorGroup::Unary ? self >= parent : self > parent;
    case OperatorPosition::Right:
        return parent == OperatorGroup::Unary ? self > parent : self >= parent;
    }
}

}  // namespace wgslx::writer

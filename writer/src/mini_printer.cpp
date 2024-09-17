#include "mini_printer.h"

#include <src/tint/lang/wgsl/ast/alias.h>
#include <src/tint/lang/wgsl/ast/binary_expression.h>
#include <src/tint/lang/wgsl/ast/blend_src_attribute.h>
#include <src/tint/lang/wgsl/ast/bool_literal_expression.h>
#include <src/tint/lang/wgsl/ast/call_expression.h>
#include <src/tint/lang/wgsl/ast/color_attribute.h>
#include <src/tint/lang/wgsl/ast/const.h>
#include <src/tint/lang/wgsl/ast/diagnostic_attribute.h>
#include <src/tint/lang/wgsl/ast/diagnostic_rule_name.h>
#include <src/tint/lang/wgsl/ast/enable.h>
#include <src/tint/lang/wgsl/ast/float_literal_expression.h>
#include <src/tint/lang/wgsl/ast/group_attribute.h>
#include <src/tint/lang/wgsl/ast/id_attribute.h>
#include <src/tint/lang/wgsl/ast/identifier.h>
#include <src/tint/lang/wgsl/ast/identifier_expression.h>
#include <src/tint/lang/wgsl/ast/index_accessor_expression.h>
#include <src/tint/lang/wgsl/ast/int_literal_expression.h>
#include <src/tint/lang/wgsl/ast/internal_attribute.h>
#include <src/tint/lang/wgsl/ast/interpolate_attribute.h>
#include <src/tint/lang/wgsl/ast/invariant_attribute.h>
#include <src/tint/lang/wgsl/ast/let.h>
#include <src/tint/lang/wgsl/ast/literal_expression.h>
#include <src/tint/lang/wgsl/ast/location_attribute.h>
#include <src/tint/lang/wgsl/ast/member_accessor_expression.h>
#include <src/tint/lang/wgsl/ast/module.h>
#include <src/tint/lang/wgsl/ast/must_use_attribute.h>
#include <src/tint/lang/wgsl/ast/override.h>
#include <src/tint/lang/wgsl/ast/phony_expression.h>
#include <src/tint/lang/wgsl/ast/stage_attribute.h>
#include <src/tint/lang/wgsl/ast/stride_attribute.h>
#include <src/tint/lang/wgsl/ast/struct.h>
#include <src/tint/lang/wgsl/ast/struct_member_align_attribute.h>
#include <src/tint/lang/wgsl/ast/struct_member_offset_attribute.h>
#include <src/tint/lang/wgsl/ast/struct_member_size_attribute.h>
#include <src/tint/lang/wgsl/ast/templated_identifier.h>
#include <src/tint/lang/wgsl/ast/type.h>
#include <src/tint/lang/wgsl/ast/type_decl.h>
#include <src/tint/lang/wgsl/ast/unary_op_expression.h>
#include <src/tint/lang/wgsl/ast/var.h>
#include <src/tint/lang/wgsl/ast/workgroup_attribute.h>
#include <src/tint/lang/wgsl/features/language_feature.h>
#include <src/tint/utils/rtti/switch.h>

#include <algorithm>
#include <range/v3/range/conversion.hpp>
#include <range/v3/view/for_each.hpp>
#include <vector>

#include "operator_group.h"

namespace wgslx::writer {

bool MiniPrinter::Generate() {
    EmitEnables();
    EmitRequires();
    EmitDiagnosticDirectives();
    for (auto* decl : program_->AST().GlobalDeclarations()) {
        if (decl->IsAnyOf<tint::ast::DiagnosticDirective, tint::ast::Enable, tint::ast::Requires>()) {
            continue;
        }
        Switch(
            decl,
            [&](const tint::ast::TypeDecl* td) { return EmitTypeDecl(td); },
            [&](const tint::ast::Function* func) { return EmitFunction(func); },
            [&](const tint::ast::Variable* var) { return EmitVariable(var); },
            [&](const tint::ast::ConstAssert* ca) { return EmitConstAssert(ca); },  //
            TINT_ICE_ON_NO_MATCH
        );
    }
    return true;
}

void MiniPrinter::EmitEnables() {
    std::vector<tint::wgsl::Extension> extensions;
    for (const auto* enable : program_->AST().Enables()) {
        for (const auto* extension : enable->extensions) {
            extensions.emplace_back(extension->name);
        }
    }
    std::sort(extensions.begin(), extensions.end(), [](tint::wgsl::Extension a, tint::wgsl::Extension b) {
        return a < b;
    });
    auto last = std::unique(extensions.begin(), extensions.end());
    extensions.erase(last, extensions.end());

    if (!extensions.empty()) {
        ss_ << "enable ";
        for (auto iter = extensions.begin(); iter != extensions.end(); ++iter) {
            if (iter != extensions.begin()) {
                ss_ << ",";
            }
            ss_ << *iter;
        }
        ss_ << ";";
    }
}

void MiniPrinter::EmitRequires() {
    std::vector<tint::wgsl::LanguageFeature> features;
    for (const auto* require : program_->AST().Requires()) {
        for (auto feature : require->features) {
            features.emplace_back(feature);
        }
    }
    std::sort(features.begin(), features.end(), [](tint::wgsl::LanguageFeature a, tint::wgsl::LanguageFeature b) {
        return a < b;
    });
    auto last = std::unique(features.begin(), features.end());
    features.erase(last, features.end());

    if (!features.empty()) {
        ss_ << "requires ";
        for (auto iter = features.begin(); iter != features.end(); ++iter) {
            if (iter != features.begin()) {
                ss_ << ",";
            }
            ss_ << tint::wgsl::ToString(*iter);
        }
        ss_ << ";";
    }
}

void MiniPrinter::EmitDiagnosticDirectives() {
    for (auto diagnostic : program_->AST().DiagnosticDirectives()) {
        EmitDiagnosticControl(diagnostic->control);
    }
}

void MiniPrinter::EmitDiagnosticControl(const tint::ast::DiagnosticControl& diagnostic) {
    ss_ << "diagnostic(" << diagnostic.severity << "," << diagnostic.rule_name->String() << ");";
}

void MiniPrinter::EmitTypeDecl(const tint::ast::TypeDecl* td) {
    Switch(
        td,
        [&](const tint::ast::Alias* alias) {
            ss_ << "alias " << alias->name->symbol.Name() << "=";
            EmitExpression(alias->type, OperatorPosition::Left, OperatorGroup::None);
            ss_ << ";";
        },
        [&](const tint::ast::Struct* str) { EmitStructType(str); },
        TINT_ICE_ON_NO_MATCH
    );
}

void MiniPrinter::EmitFunction(const tint::ast::Function* func) {}

void MiniPrinter::EmitVariable(const tint::ast::Variable* var) {
    if (!var->attributes.IsEmpty()) {
        EmitAttributes(var->attributes);
    }

    Switch(
        var,
        [&](const tint::ast::Var* var) {
            ss_ << "var";
            if (var->declared_address_space || var->declared_access) {
                ss_ << "<";
                EmitExpression(var->declared_address_space, OperatorPosition::Left, OperatorGroup::None);
                if (var->declared_access) {
                    ss_ << ",";
                    EmitExpression(var->declared_access, OperatorPosition::Left, OperatorGroup::None);
                }
                ss_ << ">";
            }
        },
        [&](const tint::ast::Let*) { ss_ << "let"; },
        [&](const tint::ast::Override*) { ss_ << "override"; },
        [&](const tint::ast::Const*) { ss_ << "const"; },  //
        TINT_ICE_ON_NO_MATCH
    );
    ss_ << " " << var->name->symbol.Name();
    if (auto ty = var->type) {
        ss_ << ":";
        EmitExpression(ty, OperatorPosition::Left, OperatorGroup::None);
    }
    if (var->initializer != nullptr) {
        ss_ << "=";
        EmitExpression(var->initializer, OperatorPosition::Left, OperatorGroup::None);
    }
    ss_ << ";";
}

void MiniPrinter::EmitConstAssert(const tint::ast::ConstAssert* ca) {}

void MiniPrinter::EmitStructType(const tint::ast::Struct* str) {}

void MiniPrinter::EmitAttributes(tint::VectorRef<const tint::ast::Attribute*> attrs) {
    for (auto* attr : attrs) {
        ss_ << "@";
        Switch(
            attr,
            [&](const tint::ast::WorkgroupAttribute* workgroup) {
                auto values = workgroup->Values();
                ss_ << "workgroup_size(";
                for (size_t i = 0; i < 3; i++) {
                    if (values[i]) {
                        if (i > 0) {
                            ss_ << ",";
                        }
                        EmitExpression(values[i], OperatorPosition::Left, OperatorGroup::None);
                    }
                }
                ss_ << ")";
            },
            [&](const tint::ast::StageAttribute* stage) {
                ss_ << stage->stage;
                ss_ << " ";
            },
            [&](const tint::ast::BindingAttribute* binding) {
                ss_ << "binding(";
                EmitExpression(binding->expr, OperatorPosition::Left, OperatorGroup::None);
                ss_ << ")";
            },
            [&](const tint::ast::GroupAttribute* group) {
                ss_ << "group(";
                EmitExpression(group->expr, OperatorPosition::Left, OperatorGroup::None);
                ss_ << ")";
            },
            [&](const tint::ast::LocationAttribute* location) {
                ss_ << "location(";
                EmitExpression(location->expr, OperatorPosition::Left, OperatorGroup::None);
                ss_ << ")";
            },
            [&](const tint::ast::ColorAttribute* color) {
                ss_ << "color(";
                EmitExpression(color->expr, OperatorPosition::Left, OperatorGroup::None);
                ss_ << ")";
            },
            [&](const tint::ast::BlendSrcAttribute* blend_src) {
                ss_ << "blend_src(";
                EmitExpression(blend_src->expr, OperatorPosition::Left, OperatorGroup::None);
                ss_ << ")";
            },
            [&](const tint::ast::BuiltinAttribute* builtin) {
                ss_ << "builtin(";
                ss_ << tint::core::ToString(builtin->builtin);
                ss_ << ")";
            },
            [&](const tint::ast::DiagnosticAttribute* diagnostic) { EmitDiagnosticControl(diagnostic->control); },
            [&](const tint::ast::InterpolateAttribute* interpolate) {
                ss_ << "interpolate(";
                ss_ << tint::core::ToString(interpolate->interpolation.type);
                if (interpolate->interpolation.sampling != tint::core::InterpolationSampling::kUndefined) {
                    ss_ << ",";
                    ss_ << tint::core::ToString(interpolate->interpolation.sampling);
                }
                ss_ << ")";
            },
            [&](const tint::ast::InvariantAttribute*) { ss_ << "invariant "; },
            [&](const tint::ast::IdAttribute* override_deco) {
                ss_ << "id(";
                EmitExpression(override_deco->expr, OperatorPosition::Left, OperatorGroup::None);
                ss_ << ")";
            },
            [&](const tint::ast::MustUseAttribute*) { ss_ << "must_use "; },
            [&](const tint::ast::StructMemberOffsetAttribute* offset) {
                ss_ << "offset(";
                EmitExpression(offset->expr, OperatorPosition::Left, OperatorGroup::None);
                ss_ << ")";
            },
            [&](const tint::ast::StructMemberSizeAttribute* size) {
                ss_ << "size(";
                EmitExpression(size->expr, OperatorPosition::Left, OperatorGroup::None);
                ss_ << ")";
            },
            [&](const tint::ast::StructMemberAlignAttribute* align) {
                ss_ << "align(";
                EmitExpression(align->expr, OperatorPosition::Left, OperatorGroup::None);
                ss_ << ")";
            },
            [&](const tint::ast::StrideAttribute* stride) {
                ss_ << "stride(";
                ss_ << stride->stride;
                ss_ << ")";
            },
            [&](const tint::ast::InternalAttribute* internal) {
                ss_ << "internal(";
                ss_ << internal->InternalName();
                ss_ << ")";
            },
            [&](const tint::ast::InputAttachmentIndexAttribute* index) {
                ss_ << "input_attachment_index(";
                EmitExpression(index->expr, OperatorPosition::Left, OperatorGroup::None);
                ss_ << ")";
            },
            TINT_ICE_ON_NO_MATCH
        );
    }
}

void MiniPrinter::EmitExpression(const tint::ast::Expression* expr, OperatorPosition position, OperatorGroup parent) {
    Switch(
        expr,
        [&](const tint::ast::IndexAccessorExpression* a) { EmitIndexAccessor(a); },
        [&](const tint::ast::BinaryExpression* b) { EmitBinary(b, position, parent); },
        [&](const tint::ast::CallExpression* c) { EmitCall(c); },
        [&](const tint::ast::IdentifierExpression* i) { EmitIdentifier(i); },
        [&](const tint::ast::LiteralExpression* l) { EmitLiteral(l); },
        [&](const tint::ast::MemberAccessorExpression* m) { EmitMemberAccessor(m); },
        [&](const tint::ast::PhonyExpression*) { ss_ << "_"; },
        [&](const tint::ast::UnaryOpExpression* u) { EmitUnaryOp(u, position, parent); },
        TINT_ICE_ON_NO_MATCH
    );
}

void MiniPrinter::EmitIndexAccessor(const tint::ast::IndexAccessorExpression* expr) {
    bool paren_lhs =
        !expr->object
             ->IsAnyOf<tint::ast::AccessorExpression, tint::ast::CallExpression, tint::ast::IdentifierExpression>();
    if (paren_lhs) {
        ss_ << "(";
    }
    EmitExpression(expr->object, OperatorPosition::Left, OperatorGroup::None);
    if (paren_lhs) {
        ss_ << ")";
    }
    ss_ << "[";
    EmitExpression(expr->index, OperatorPosition::Left, OperatorGroup::None);
    ss_ << "]";
}

void MiniPrinter::EmitMemberAccessor(const tint::ast::MemberAccessorExpression* expr) {
    bool paren_lhs =
        !expr->object
             ->IsAnyOf<tint::ast::AccessorExpression, tint::ast::CallExpression, tint::ast::IdentifierExpression>();
    if (paren_lhs) {
        ss_ << "(";
    }
    EmitExpression(expr->object, OperatorPosition::Left, OperatorGroup::None);
    if (paren_lhs) {
        ss_ << ")";
    }
    ss_ << "." << expr->member->symbol.Name();
}

void MiniPrinter::EmitBinary(const tint::ast::BinaryExpression* expr, OperatorPosition position, OperatorGroup parent) {
    auto group = toOperatorGroup(expr->op);
    auto paren = isParenthesisRequired(group, position, parent);
    if (paren) {
        ss_ << "(";
    }
    EmitExpression(expr->lhs, OperatorPosition::Left, group);
    EmitBinaryOp(expr->op);
    EmitExpression(expr->rhs, OperatorPosition::Right, group);
    if (paren) {
        ss_ << ")";
    }
}

void MiniPrinter::EmitBinaryOp(const tint::core::BinaryOp op) {
    switch (op) {
    case tint::core::BinaryOp::kAnd:
        ss_ << "&";
        return;
    case tint::core::BinaryOp::kOr:
        ss_ << "|";
        return;
    case tint::core::BinaryOp::kXor:
        ss_ << "^";
        return;
    case tint::core::BinaryOp::kLogicalAnd:
        ss_ << "&&";
        return;
    case tint::core::BinaryOp::kLogicalOr:
        ss_ << "||";
        return;
    case tint::core::BinaryOp::kEqual:
        ss_ << "==";
        return;
    case tint::core::BinaryOp::kNotEqual:
        ss_ << "!=";
        return;
    case tint::core::BinaryOp::kLessThan:
        ss_ << "<";
        return;
    case tint::core::BinaryOp::kGreaterThan:
        ss_ << ">";
        return;
    case tint::core::BinaryOp::kLessThanEqual:
        ss_ << "<=";
        return;
    case tint::core::BinaryOp::kGreaterThanEqual:
        ss_ << ">=";
        return;
    case tint::core::BinaryOp::kShiftLeft:
        ss_ << "<<";
        return;
    case tint::core::BinaryOp::kShiftRight:
        ss_ << ">>";
        return;
    case tint::core::BinaryOp::kAdd:
        ss_ << "+";
        return;
    case tint::core::BinaryOp::kSubtract:
        ss_ << "-";
        return;
    case tint::core::BinaryOp::kMultiply:
        ss_ << "*";
        return;
    case tint::core::BinaryOp::kDivide:
        ss_ << "/";
        return;
    case tint::core::BinaryOp::kModulo:
        ss_ << "%";
        return;
    }
    TINT_ICE() << "invalid binary op " << op;
}

void MiniPrinter::EmitCall(const tint::ast::CallExpression* expr) {
    EmitExpression(expr->target, OperatorPosition::Left, OperatorGroup::Primary);
    ss_ << "(";
    bool first = true;
    const auto& args = expr->args;
    for (auto* arg : args) {
        if (!first) {
            ss_ << ",";
        }
        first = false;
        EmitExpression(arg, OperatorPosition::Left, OperatorGroup::None);
    }
    ss_ << ")";
}

void MiniPrinter::EmitIdentifier(const tint::ast::IdentifierExpression* expr) {
    EmitIdentifier(expr->identifier);
}

void MiniPrinter::EmitIdentifier(const tint::ast::Identifier* ident) {
    if (auto* tmpl_ident = ident->As<tint::ast::TemplatedIdentifier>()) {
        if (!tmpl_ident->attributes.IsEmpty()) {
            EmitAttributes(tmpl_ident->attributes);
        }
        ss_ << ident->symbol.Name() << "<";
        for (auto* expr : tmpl_ident->arguments) {
            if (expr != tmpl_ident->arguments.Front()) {
                ss_ << ",";
            }
            EmitExpression(expr, OperatorPosition::Left, OperatorGroup::None);
        }
        ss_ << ">";
    } else {
        ss_ << ident->symbol.Name();
    }
}

void MiniPrinter::EmitLiteral(const tint::ast::LiteralExpression* lit) {
    Switch(
        lit,
        [&](const tint::ast::BoolLiteralExpression* l) { ss_ << (l->value ? "true" : "false"); },
        [&](const tint::ast::FloatLiteralExpression* l) { ss_ << l->value << l->suffix; },
        [&](const tint::ast::IntLiteralExpression* l) { ss_ << l->value << l->suffix; },  //
        TINT_ICE_ON_NO_MATCH
    );
}

void MiniPrinter::EmitUnaryOp(
    const tint::ast::UnaryOpExpression* expr,
    OperatorPosition position,
    OperatorGroup parent
) {
    auto paren = isParenthesisRequired(OperatorGroup::Unary, position, parent);
    if (paren) {
        ss_ << "(";
    }
    switch (expr->op) {
    case tint::core::UnaryOp::kAddressOf:
        ss_ << "&";
        break;
    case tint::core::UnaryOp::kComplement:
        ss_ << "~";
        break;
    case tint::core::UnaryOp::kIndirection:
        ss_ << "*";
        break;
    case tint::core::UnaryOp::kNot:
        ss_ << "!";
        break;
    case tint::core::UnaryOp::kNegation:
        ss_ << "-";
        break;
    }
    EmitExpression(expr->expr, OperatorPosition::Right, OperatorGroup::Unary);
    if (paren) {
        ss_ << ")";
    }
}

std::string MiniPrinter::Result() {
    return std::move(ss_).str();
}

}  // namespace wgslx::writer

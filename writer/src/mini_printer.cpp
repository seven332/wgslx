#include "mini_printer.h"

#include <__format/format_functions.h>
#include <src/tint/lang/wgsl/ast/alias.h>
#include <src/tint/lang/wgsl/ast/assignment_statement.h>
#include <src/tint/lang/wgsl/ast/binary_expression.h>
#include <src/tint/lang/wgsl/ast/blend_src_attribute.h>
#include <src/tint/lang/wgsl/ast/block_statement.h>
#include <src/tint/lang/wgsl/ast/bool_literal_expression.h>
#include <src/tint/lang/wgsl/ast/break_if_statement.h>
#include <src/tint/lang/wgsl/ast/break_statement.h>
#include <src/tint/lang/wgsl/ast/call_expression.h>
#include <src/tint/lang/wgsl/ast/call_statement.h>
#include <src/tint/lang/wgsl/ast/color_attribute.h>
#include <src/tint/lang/wgsl/ast/compound_assignment_statement.h>
#include <src/tint/lang/wgsl/ast/const.h>
#include <src/tint/lang/wgsl/ast/const_assert.h>
#include <src/tint/lang/wgsl/ast/continue_statement.h>
#include <src/tint/lang/wgsl/ast/diagnostic_attribute.h>
#include <src/tint/lang/wgsl/ast/diagnostic_rule_name.h>
#include <src/tint/lang/wgsl/ast/discard_statement.h>
#include <src/tint/lang/wgsl/ast/enable.h>
#include <src/tint/lang/wgsl/ast/float_literal_expression.h>
#include <src/tint/lang/wgsl/ast/for_loop_statement.h>
#include <src/tint/lang/wgsl/ast/group_attribute.h>
#include <src/tint/lang/wgsl/ast/id_attribute.h>
#include <src/tint/lang/wgsl/ast/identifier.h>
#include <src/tint/lang/wgsl/ast/identifier_expression.h>
#include <src/tint/lang/wgsl/ast/if_statement.h>
#include <src/tint/lang/wgsl/ast/increment_decrement_statement.h>
#include <src/tint/lang/wgsl/ast/index_accessor_expression.h>
#include <src/tint/lang/wgsl/ast/int_literal_expression.h>
#include <src/tint/lang/wgsl/ast/internal_attribute.h>
#include <src/tint/lang/wgsl/ast/interpolate_attribute.h>
#include <src/tint/lang/wgsl/ast/invariant_attribute.h>
#include <src/tint/lang/wgsl/ast/let.h>
#include <src/tint/lang/wgsl/ast/literal_expression.h>
#include <src/tint/lang/wgsl/ast/location_attribute.h>
#include <src/tint/lang/wgsl/ast/loop_statement.h>
#include <src/tint/lang/wgsl/ast/member_accessor_expression.h>
#include <src/tint/lang/wgsl/ast/module.h>
#include <src/tint/lang/wgsl/ast/must_use_attribute.h>
#include <src/tint/lang/wgsl/ast/override.h>
#include <src/tint/lang/wgsl/ast/phony_expression.h>
#include <src/tint/lang/wgsl/ast/return_statement.h>
#include <src/tint/lang/wgsl/ast/stage_attribute.h>
#include <src/tint/lang/wgsl/ast/stride_attribute.h>
#include <src/tint/lang/wgsl/ast/struct.h>
#include <src/tint/lang/wgsl/ast/struct_member_align_attribute.h>
#include <src/tint/lang/wgsl/ast/struct_member_offset_attribute.h>
#include <src/tint/lang/wgsl/ast/struct_member_size_attribute.h>
#include <src/tint/lang/wgsl/ast/switch_statement.h>
#include <src/tint/lang/wgsl/ast/templated_identifier.h>
#include <src/tint/lang/wgsl/ast/type.h>
#include <src/tint/lang/wgsl/ast/type_decl.h>
#include <src/tint/lang/wgsl/ast/unary_op_expression.h>
#include <src/tint/lang/wgsl/ast/var.h>
#include <src/tint/lang/wgsl/ast/variable_decl_statement.h>
#include <src/tint/lang/wgsl/ast/while_statement.h>
#include <src/tint/lang/wgsl/ast/workgroup_attribute.h>
#include <src/tint/lang/wgsl/features/language_feature.h>
#include <src/tint/lang/wgsl/sem/struct.h>
#include <src/tint/utils/rtti/switch.h>
#include <src/tint/utils/strconv/float_to_string.h>
#include <src/tint/utils/text/string.h>

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
        ss_ << ";";
    }
}

void MiniPrinter::EmitDiagnosticControl(const tint::ast::DiagnosticControl& diagnostic) {
    ss_ << "diagnostic(" << diagnostic.severity << "," << diagnostic.rule_name->String() << ")";
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

void MiniPrinter::EmitFunction(const tint::ast::Function* func) {
    EmitAttributes(func->attributes);

    ss_ << "fn " << func->name->symbol.Name() << "(";
    bool first = true;
    for (auto* v : func->params) {
        if (!first) {
            ss_ << ",";
        }
        first = false;

        EmitAttributes(v->attributes);
        ss_ << v->name->symbol.Name() << ":";
        EmitExpression(v->type, OperatorPosition::Left, OperatorGroup::None);
    }
    ss_ << ")";

    if (func->return_type || !func->return_type_attributes.IsEmpty()) {
        ss_ << "->";
        EmitAttributes(func->return_type_attributes);
        EmitExpression(func->return_type, OperatorPosition::Left, OperatorGroup::None);
    }

    if (func->body) {
        EmitStatement(func->body);
    }
}

void MiniPrinter::EmitStatement(const tint::ast::Statement* stmt) {
    Switch(
        stmt,
        [&](const tint::ast::AssignmentStatement* a) { EmitAssign(a); },
        [&](const tint::ast::BlockStatement* b) { EmitBlock(b); },
        [&](const tint::ast::BreakStatement* b) { ss_ << "break;"; },
        [&](const tint::ast::BreakIfStatement* b) { EmitBreakIf(b); },
        [&](const tint::ast::CallStatement* c) {
            EmitCall(c->expr);
            ss_ << ";";
        },
        [&](const tint::ast::CompoundAssignmentStatement* c) { EmitCompoundAssign(c); },
        [&](const tint::ast::ContinueStatement* c) { ss_ << "continue;"; },
        [&](const tint::ast::DiscardStatement* d) { ss_ << "discard;"; },
        [&](const tint::ast::IfStatement* i) { EmitIf(i); },
        [&](const tint::ast::IncrementDecrementStatement* l) { EmitIncrementDecrement(l); },
        [&](const tint::ast::LoopStatement* l) { EmitLoop(l); },
        [&](const tint::ast::ForLoopStatement* l) { EmitForLoop(l); },
        [&](const tint::ast::WhileStatement* l) { EmitWhile(l); },
        [&](const tint::ast::ReturnStatement* r) { EmitReturn(r); },
        [&](const tint::ast::ConstAssert* c) { EmitConstAssert(c); },
        [&](const tint::ast::SwitchStatement* s) { EmitSwitch(s); },
        [&](const tint::ast::VariableDeclStatement* v) { EmitVariable(v->variable); },
        TINT_ICE_ON_NO_MATCH
    );
}

void MiniPrinter::EmitAssign(const tint::ast::AssignmentStatement* stmt) {
    EmitExpression(stmt->lhs, OperatorPosition::Left, OperatorGroup::None);
    ss_ << "=";
    EmitExpression(stmt->rhs, OperatorPosition::Left, OperatorGroup::None);
    ss_ << ";";
}

void MiniPrinter::EmitBlock(const tint::ast::BlockStatement* stmt) {
    EmitAttributes(stmt->attributes);
    ss_ << "{";
    for (auto* s : stmt->statements) {
        EmitStatement(s);
    }
    ss_ << "}";
}

void MiniPrinter::EmitBreakIf(const tint::ast::BreakIfStatement* b) {
    ss_ << "break if ";
    EmitExpression(b->condition, OperatorPosition::Left, OperatorGroup::None);
    ss_ << ";";
}

void MiniPrinter::EmitCompoundAssign(const tint::ast::CompoundAssignmentStatement* stmt) {
    EmitExpression(stmt->lhs, OperatorPosition::Left, OperatorGroup::None);
    EmitBinaryOp(stmt->op);
    ss_ << "=";
    EmitExpression(stmt->rhs, OperatorPosition::Left, OperatorGroup::None);
    ss_ << ";";
}

void MiniPrinter::EmitIf(const tint::ast::IfStatement* stmt) {
    EmitAttributes(stmt->attributes);
    ss_ << "if(";
    EmitExpression(stmt->condition, OperatorPosition::Left, OperatorGroup::None);
    ss_ << ")";
    EmitStatement(stmt->body);
    const auto* e = stmt->else_statement;
    while (e) {
        if (auto* elseif = e->As<tint::ast::IfStatement>()) {
            ss_ << "else if(";
            EmitExpression(elseif->condition, OperatorPosition::Left, OperatorGroup::None);
            ss_ << ")";
            EmitStatement(elseif->body);
            e = elseif->else_statement;
        } else {
            auto* body = e->As<tint::ast::BlockStatement>();
            ss_ << "else";
            EmitStatement(body);
            break;
        }
    }
}

void MiniPrinter::EmitIncrementDecrement(const tint::ast::IncrementDecrementStatement* stmt) {
    EmitExpression(stmt->lhs, OperatorPosition::Left, OperatorGroup::None);
    ss_ << (stmt->increment ? "++" : "--") << ";";
}

void MiniPrinter::EmitLoop(const tint::ast::LoopStatement* stmt) {
    EmitAttributes(stmt->attributes);
    ss_ << "loop";
    EmitAttributes(stmt->body->attributes);
    ss_ << "{";
    for (auto* s : stmt->body->statements) {
        EmitStatement(s);
    }
    if (stmt->continuing && !stmt->continuing->Empty()) {
        ss_ << "continuing";
        EmitAttributes(stmt->continuing->attributes);
        ss_ << "{";
        for (auto* s : stmt->continuing->statements) {
            EmitStatement(s);
        }
        ss_ << "}";
    }
    ss_ << "}";
}

void MiniPrinter::EmitForLoop(const tint::ast::ForLoopStatement* stmt) {
    EmitAttributes(stmt->attributes);
    ss_ << "for(";
    if (stmt->initializer) {
        EmitStatement(stmt->initializer);
    } else {
        ss_ << ";";
    }
    if (auto* cond = stmt->condition) {
        EmitExpression(cond, OperatorPosition::Left, OperatorGroup::None);
    }
    ss_ << ";";
    if (stmt->continuing) {
        EmitStatement(stmt->continuing);
        ss_.seekp(-1, std::ios_base::end);
    }
    ss_ << ")";
    EmitStatement(stmt->body);
}

void MiniPrinter::EmitWhile(const tint::ast::WhileStatement* stmt) {
    EmitAttributes(stmt->attributes);
    ss_ << "while(";
    EmitExpression(stmt->condition, OperatorPosition::Left, OperatorGroup::None);
    ss_ << ")";
    EmitStatement(stmt->body);
}

void MiniPrinter::EmitReturn(const tint::ast::ReturnStatement* stmt) {
    ss_ << "return";
    if (stmt->value) {
        ss_ << " ";
        EmitExpression(stmt->value, OperatorPosition::Left, OperatorGroup::None);
    }
    ss_ << ";";
}

void MiniPrinter::EmitSwitch(const tint::ast::SwitchStatement* stmt) {
    EmitAttributes(stmt->attributes);
    ss_ << "switch(";
    EmitExpression(stmt->condition, OperatorPosition::Left, OperatorGroup::None);
    ss_ << ")";
    EmitAttributes(stmt->body_attributes);
    ss_ << "{";
    for (auto* s : stmt->body) {
        EmitCase(s);
    }
    ss_ << "}";
}

void MiniPrinter::EmitCase(const tint::ast::CaseStatement* stmt) {
    if (stmt->selectors.Length() == 1 && stmt->ContainsDefault()) {
        ss_ << "default";
    } else {
        ss_ << "case ";
        bool first = true;
        for (auto* sel : stmt->selectors) {
            if (!first) {
                ss_ << ",";
            }
            first = false;
            if (sel->IsDefault()) {
                ss_ << "default";
            } else {
                EmitExpression(sel->expr, OperatorPosition::Left, OperatorGroup::None);
            }
        }
    }
    EmitStatement(stmt->body);
}

void MiniPrinter::EmitVariable(const tint::ast::Variable* var) {
    EmitAttributes(var->attributes);

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

void MiniPrinter::EmitConstAssert(const tint::ast::ConstAssert* ca) {
    ss_ << "const_assert ";
    EmitExpression(ca->condition, OperatorPosition::Left, OperatorGroup::None);
    ss_ << ";";
}

void MiniPrinter::EmitStructType(const tint::ast::Struct* str) {
    EmitAttributes(str->attributes);
    ss_ << "struct " << str->name->symbol.Name() << "{";

    tint::Hashset<std::string, 8> member_names;
    for (auto* mem : str->members) {
        member_names.Add(std::string(mem->name->symbol.NameView()));
    }
    size_t padding_idx = 0;
    auto new_padding_name = [&] {
        while (true) {
            auto name = "padding_" + tint::ToString(padding_idx++);
            if (member_names.Add(name)) {
                return name;
            }
        }
    };

    auto add_padding = [&](uint32_t size) {
        ss_ << "@size(" << size << ")";
        // Note: u32 is the smallest primitive we currently support. When WGSL
        // supports smaller types, this will need to be updated.
        ss_ << new_padding_name() << ":u32,";
    };

    uint32_t offset = 0;
    for (auto* mem : str->members) {
        // TODO(crbug.com/tint/798) move the @offset attribute handling to the transform::Wgsl
        // sanitizer.
        if (auto* mem_sem = program_->Sem().Get(mem)) {
            offset = tint::RoundUp(mem_sem->Align(), offset);
            if (uint32_t padding = mem_sem->Offset() - offset) {
                add_padding(padding);
                offset += padding;
            }
            offset += mem_sem->Size();
        }

        // Offset attributes no longer exist in the WGSL spec, but are emitted
        // by the SPIR-V reader and are consumed by the Resolver(). These should not
        // be emitted, but instead struct padding fields should be emitted.
        tint::Vector<const tint::ast::Attribute*, 4> attributes_sanitized;
        attributes_sanitized.Reserve(mem->attributes.Length());
        for (auto* attr : mem->attributes) {
            if (attr->Is<tint::ast::StructMemberOffsetAttribute>()) {
                // Skip
            } else {
                attributes_sanitized.Push(attr);
            }
        }
        EmitAttributes(attributes_sanitized);

        ss_ << mem->name->symbol.Name() << ":";
        EmitExpression(mem->type, OperatorPosition::Left, OperatorGroup::None);
        if (mem != str->members.Back()) {
            ss_ << ",";
        }
    }
    ss_ << "}";
}

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
        EmitAttributes(tmpl_ident->attributes);
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
        [&](const tint::ast::FloatLiteralExpression* l) {
            if (options_->precise_float) {
                if (l->suffix == tint::ast::FloatLiteralExpression::Suffix::kNone) {
                    ss_ << tint::strconv::DoubleToBitPreservingString(l->value);
                } else {
                    ss_ << tint::strconv::FloatToBitPreservingString(static_cast<float>(l->value)) << l->suffix;
                }
            } else {
                auto str = std::format("{:.6f}", l->value);
                while (str.ends_with('0')) {
                    str.pop_back();
                }
                ss_ << str << l->suffix;
            }
        },
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

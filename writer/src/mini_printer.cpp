#include "mini_printer.h"

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
#include <format>
#include <range/v3/range/conversion.hpp>
#include <range/v3/view/for_each.hpp>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "operator_group.h"

namespace wgslx::writer {

bool MiniPrinter::Generate() {
    EmitEnables(ss_);
    EmitRequires(ss_);
    EmitDiagnosticDirectives(ss_);
    for (auto* decl : program_->AST().GlobalDeclarations()) {
        if (decl->IsAnyOf<tint::ast::DiagnosticDirective, tint::ast::Enable, tint::ast::Requires>()) {
            continue;
        }
        Switch(
            decl,
            [&](const tint::ast::TypeDecl* td) { return EmitTypeDecl(ss_, td); },
            [&](const tint::ast::Function* func) { return EmitFunction(ss_, func); },
            [&](const tint::ast::Variable* var) { return EmitVariable(ss_, var); },
            [&](const tint::ast::ConstAssert* ca) { return EmitConstAssert(ss_, ca); },  //
            TINT_ICE_ON_NO_MATCH
        );
    }
    return true;
}

void MiniPrinter::EmitEnables(std::stringstream& out) {
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
        out << "enable ";
        for (auto iter = extensions.begin(); iter != extensions.end(); ++iter) {
            if (iter != extensions.begin()) {
                out << ",";
            }
            out << *iter;
        }
        out << ";";
    }
}

void MiniPrinter::EmitRequires(std::stringstream& out) {
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
        out << "requires ";
        for (auto iter = features.begin(); iter != features.end(); ++iter) {
            if (iter != features.begin()) {
                out << ",";
            }
            out << tint::wgsl::ToString(*iter);
        }
        out << ";";
    }
}

void MiniPrinter::EmitDiagnosticDirectives(std::stringstream& out) {
    for (auto diagnostic : program_->AST().DiagnosticDirectives()) {
        EmitDiagnosticControl(out, diagnostic->control);
        out << ";";
    }
}

void MiniPrinter::EmitDiagnosticControl(std::stringstream& out, const tint::ast::DiagnosticControl& diagnostic) {
    out << "diagnostic(" << diagnostic.severity << "," << diagnostic.rule_name->String() << ")";
}

void MiniPrinter::EmitTypeDecl(std::stringstream& out, const tint::ast::TypeDecl* td) {
    Switch(
        td,
        [&](const tint::ast::Alias* alias) {
            out << "alias " << alias->name->symbol.Name() << "=";
            EmitExpression(out, alias->type, OperatorPosition::Left, OperatorGroup::None);
            out << ";";
        },
        [&](const tint::ast::Struct* str) { EmitStructType(out, str); },
        TINT_ICE_ON_NO_MATCH
    );
}

void MiniPrinter::EmitFunction(std::stringstream& out, const tint::ast::Function* func) {
    EmitAttributes(out, func->attributes);

    out << "fn " << func->name->symbol.Name() << "(";
    bool first = true;
    for (auto* v : func->params) {
        if (!first) {
            out << ",";
        }
        first = false;

        EmitAttributes(out, v->attributes);
        out << v->name->symbol.Name() << ":";
        EmitExpression(out, v->type, OperatorPosition::Left, OperatorGroup::None);
    }
    out << ")";

    if (func->return_type || !func->return_type_attributes.IsEmpty()) {
        out << "->";
        EmitAttributes(out, func->return_type_attributes);
        EmitExpression(out, func->return_type, OperatorPosition::Left, OperatorGroup::None);
    }

    if (func->body) {
        EmitStatement(out, func->body);
    }
}

void MiniPrinter::EmitStatement(std::stringstream& out, const tint::ast::Statement* stmt) {
    Switch(
        stmt,
        [&](const tint::ast::AssignmentStatement* a) { EmitAssign(out, a); },
        [&](const tint::ast::BlockStatement* b) { EmitBlock(out, b); },
        [&](const tint::ast::BreakStatement*) { out << "break;"; },
        [&](const tint::ast::BreakIfStatement* b) { EmitBreakIf(out, b); },
        [&](const tint::ast::CallStatement* c) {
            EmitCall(out, c->expr);
            out << ";";
        },
        [&](const tint::ast::CompoundAssignmentStatement* c) { EmitCompoundAssign(out, c); },
        [&](const tint::ast::ContinueStatement*) { out << "continue;"; },
        [&](const tint::ast::DiscardStatement*) { out << "discard;"; },
        [&](const tint::ast::IfStatement* i) { EmitIf(out, i); },
        [&](const tint::ast::IncrementDecrementStatement* l) { EmitIncrementDecrement(out, l); },
        [&](const tint::ast::LoopStatement* l) { EmitLoop(out, l); },
        [&](const tint::ast::ForLoopStatement* l) { EmitForLoop(out, l); },
        [&](const tint::ast::WhileStatement* l) { EmitWhile(out, l); },
        [&](const tint::ast::ReturnStatement* r) { EmitReturn(out, r); },
        [&](const tint::ast::ConstAssert* c) { EmitConstAssert(out, c); },
        [&](const tint::ast::SwitchStatement* s) { EmitSwitch(out, s); },
        [&](const tint::ast::VariableDeclStatement* v) { EmitVariable(out, v->variable); },
        TINT_ICE_ON_NO_MATCH
    );
}

void MiniPrinter::EmitAssign(std::stringstream& out, const tint::ast::AssignmentStatement* stmt) {
    EmitExpression(out, stmt->lhs, OperatorPosition::Left, OperatorGroup::None);
    out << "=";
    EmitExpression(out, stmt->rhs, OperatorPosition::Left, OperatorGroup::None);
    out << ";";
}

void MiniPrinter::EmitBlock(std::stringstream& out, const tint::ast::BlockStatement* stmt) {
    EmitAttributes(out, stmt->attributes);
    out << "{";
    for (auto* s : stmt->statements) {
        EmitStatement(out, s);
    }
    out << "}";
}

void MiniPrinter::EmitBreakIf(std::stringstream& out, const tint::ast::BreakIfStatement* b) {
    out << "break if ";
    EmitExpression(out, b->condition, OperatorPosition::Left, OperatorGroup::None);
    out << ";";
}

void MiniPrinter::EmitCompoundAssign(std::stringstream& out, const tint::ast::CompoundAssignmentStatement* stmt) {
    EmitExpression(out, stmt->lhs, OperatorPosition::Left, OperatorGroup::None);
    EmitBinaryOp(out, stmt->op);
    out << "=";
    EmitExpression(out, stmt->rhs, OperatorPosition::Left, OperatorGroup::None);
    out << ";";
}

void MiniPrinter::EmitIf(std::stringstream& out, const tint::ast::IfStatement* stmt) {
    EmitAttributes(out, stmt->attributes);
    out << "if(";
    EmitExpression(out, stmt->condition, OperatorPosition::Left, OperatorGroup::None);
    out << ")";
    EmitStatement(out, stmt->body);
    const auto* e = stmt->else_statement;
    while (e) {
        if (auto* elseif = e->As<tint::ast::IfStatement>()) {
            out << "else if(";
            EmitExpression(out, elseif->condition, OperatorPosition::Left, OperatorGroup::None);
            out << ")";
            EmitStatement(out, elseif->body);
            e = elseif->else_statement;
        } else {
            auto* body = e->As<tint::ast::BlockStatement>();
            out << "else";
            EmitStatement(out, body);
            break;
        }
    }
}

void MiniPrinter::EmitIncrementDecrement(std::stringstream& out, const tint::ast::IncrementDecrementStatement* stmt) {
    EmitExpression(out, stmt->lhs, OperatorPosition::Left, OperatorGroup::None);
    out << (stmt->increment ? "++" : "--") << ";";
}

void MiniPrinter::EmitLoop(std::stringstream& out, const tint::ast::LoopStatement* stmt) {
    EmitAttributes(out, stmt->attributes);
    out << "loop";
    EmitAttributes(out, stmt->body->attributes);
    out << "{";
    for (auto* s : stmt->body->statements) {
        EmitStatement(out, s);
    }
    if (stmt->continuing && !stmt->continuing->Empty()) {
        out << "continuing";
        EmitAttributes(out, stmt->continuing->attributes);
        out << "{";
        for (auto* s : stmt->continuing->statements) {
            EmitStatement(out, s);
        }
        out << "}";
    }
    out << "}";
}

void MiniPrinter::EmitForLoop(std::stringstream& out, const tint::ast::ForLoopStatement* stmt) {
    EmitAttributes(out, stmt->attributes);
    out << "for(";
    if (stmt->initializer) {
        EmitStatement(out, stmt->initializer);
    } else {
        out << ";";
    }
    if (auto* cond = stmt->condition) {
        EmitExpression(out, cond, OperatorPosition::Left, OperatorGroup::None);
    }
    out << ";";
    if (stmt->continuing) {
        EmitStatement(out, stmt->continuing);
        out.seekp(-1, std::ios_base::end);
    }
    out << ")";
    EmitStatement(out, stmt->body);
}

void MiniPrinter::EmitWhile(std::stringstream& out, const tint::ast::WhileStatement* stmt) {
    EmitAttributes(out, stmt->attributes);
    out << "while(";
    EmitExpression(out, stmt->condition, OperatorPosition::Left, OperatorGroup::None);
    out << ")";
    EmitStatement(out, stmt->body);
}

void MiniPrinter::EmitReturn(std::stringstream& out, const tint::ast::ReturnStatement* stmt) {
    out << "return";
    if (stmt->value) {
        out << " ";
        EmitExpression(out, stmt->value, OperatorPosition::Left, OperatorGroup::None);
    }
    out << ";";
}

void MiniPrinter::EmitSwitch(std::stringstream& out, const tint::ast::SwitchStatement* stmt) {
    EmitAttributes(out, stmt->attributes);
    out << "switch(";
    EmitExpression(out, stmt->condition, OperatorPosition::Left, OperatorGroup::None);
    out << ")";
    EmitAttributes(out, stmt->body_attributes);
    out << "{";
    for (auto* s : stmt->body) {
        EmitCase(out, s);
    }
    out << "}";
}

void MiniPrinter::EmitCase(std::stringstream& out, const tint::ast::CaseStatement* stmt) {
    if (stmt->selectors.Length() == 1 && stmt->ContainsDefault()) {
        out << "default";
    } else {
        out << "case ";
        bool first = true;
        for (auto* sel : stmt->selectors) {
            if (!first) {
                out << ",";
            }
            first = false;
            if (sel->IsDefault()) {
                out << "default";
            } else {
                EmitExpression(out, sel->expr, OperatorPosition::Left, OperatorGroup::None);
            }
        }
    }
    EmitStatement(out, stmt->body);
}

void MiniPrinter::EmitVariable(std::stringstream& out, const tint::ast::Variable* var) {
    EmitAttributes(out, var->attributes);

    Switch(
        var,
        [&](const tint::ast::Var* var) {
            out << "var";
            if (var->declared_address_space || var->declared_access) {
                out << "<";
                EmitExpression(out, var->declared_address_space, OperatorPosition::Left, OperatorGroup::None);
                if (var->declared_access) {
                    out << ",";
                    EmitExpression(out, var->declared_access, OperatorPosition::Left, OperatorGroup::None);
                }
                out << ">";
            } else {
                out << " ";
            }
        },
        [&](const tint::ast::Let*) { out << "let "; },
        [&](const tint::ast::Override*) { out << "override "; },
        [&](const tint::ast::Const*) { out << "const "; },
        TINT_ICE_ON_NO_MATCH
    );

    out << var->name->symbol.Name();
    if (auto ty = var->type) {
        out << ":";
        EmitExpression(out, ty, OperatorPosition::Left, OperatorGroup::None);
    }
    if (var->initializer != nullptr) {
        out << "=";
        EmitExpression(out, var->initializer, OperatorPosition::Left, OperatorGroup::None);
    }
    out << ";";
}

void MiniPrinter::EmitConstAssert(std::stringstream& out, const tint::ast::ConstAssert* ca) {
    out << "const_assert ";
    EmitExpression(out, ca->condition, OperatorPosition::Left, OperatorGroup::None);
    out << ";";
}

void MiniPrinter::EmitStructType(std::stringstream& out, const tint::ast::Struct* str) {
    EmitAttributes(out, str->attributes);
    out << "struct " << str->name->symbol.Name() << "{";

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
        out << "@size(" << size << ")";
        // Note: u32 is the smallest primitive we currently support. When WGSL
        // supports smaller types, this will need to be updated.
        out << new_padding_name() << ":u32,";
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
        EmitAttributes(out, attributes_sanitized);

        out << mem->name->symbol.Name() << ":";
        EmitExpression(out, mem->type, OperatorPosition::Left, OperatorGroup::None);
        if (mem != str->members.Back()) {
            out << ",";
        }
    }
    out << "}";
}

void MiniPrinter::EmitAttributes(std::stringstream& out, const tint::VectorRef<const tint::ast::Attribute*>& attrs) {
    for (auto* attr : attrs) {
        out << "@";
        Switch(
            attr,
            [&](const tint::ast::WorkgroupAttribute* workgroup) {
                auto values = workgroup->Values();
                out << "workgroup_size(";
                for (size_t i = 0; i < 3; i++) {
                    if (values[i]) {
                        if (i > 0) {
                            out << ",";
                        }
                        EmitExpression(out, values[i], OperatorPosition::Left, OperatorGroup::None);
                    }
                }
                out << ")";
            },
            [&](const tint::ast::StageAttribute* stage) {
                out << stage->stage;
                out << " ";
            },
            [&](const tint::ast::BindingAttribute* binding) {
                out << "binding(";
                EmitExpression(out, binding->expr, OperatorPosition::Left, OperatorGroup::None);
                out << ")";
            },
            [&](const tint::ast::GroupAttribute* group) {
                out << "group(";
                EmitExpression(out, group->expr, OperatorPosition::Left, OperatorGroup::None);
                out << ")";
            },
            [&](const tint::ast::LocationAttribute* location) {
                out << "location(";
                EmitExpression(out, location->expr, OperatorPosition::Left, OperatorGroup::None);
                out << ")";
            },
            [&](const tint::ast::ColorAttribute* color) {
                out << "color(";
                EmitExpression(out, color->expr, OperatorPosition::Left, OperatorGroup::None);
                out << ")";
            },
            [&](const tint::ast::BlendSrcAttribute* blend_src) {
                out << "blend_src(";
                EmitExpression(out, blend_src->expr, OperatorPosition::Left, OperatorGroup::None);
                out << ")";
            },
            [&](const tint::ast::BuiltinAttribute* builtin) {
                out << "builtin(";
                out << tint::core::ToString(builtin->builtin);
                out << ")";
            },
            [&](const tint::ast::DiagnosticAttribute* diagnostic) { EmitDiagnosticControl(out, diagnostic->control); },
            [&](const tint::ast::InterpolateAttribute* interpolate) {
                out << "interpolate(";
                out << tint::core::ToString(interpolate->interpolation.type);
                if (interpolate->interpolation.sampling != tint::core::InterpolationSampling::kUndefined) {
                    out << ",";
                    out << tint::core::ToString(interpolate->interpolation.sampling);
                }
                out << ")";
            },
            [&](const tint::ast::InvariantAttribute*) { out << "invariant "; },
            [&](const tint::ast::IdAttribute* override_deco) {
                out << "id(";
                EmitExpression(out, override_deco->expr, OperatorPosition::Left, OperatorGroup::None);
                out << ")";
            },
            [&](const tint::ast::MustUseAttribute*) { out << "must_use "; },
            [&](const tint::ast::StructMemberOffsetAttribute* offset) {
                out << "offset(";
                EmitExpression(out, offset->expr, OperatorPosition::Left, OperatorGroup::None);
                out << ")";
            },
            [&](const tint::ast::StructMemberSizeAttribute* size) {
                out << "size(";
                EmitExpression(out, size->expr, OperatorPosition::Left, OperatorGroup::None);
                out << ")";
            },
            [&](const tint::ast::StructMemberAlignAttribute* align) {
                out << "align(";
                EmitExpression(out, align->expr, OperatorPosition::Left, OperatorGroup::None);
                out << ")";
            },
            [&](const tint::ast::StrideAttribute* stride) {
                out << "stride(";
                out << stride->stride;
                out << ")";
            },
            [&](const tint::ast::InternalAttribute* internal) {
                out << "internal(";
                out << internal->InternalName();
                out << ")";
            },
            [&](const tint::ast::InputAttachmentIndexAttribute* index) {
                out << "input_attachment_index(";
                EmitExpression(out, index->expr, OperatorPosition::Left, OperatorGroup::None);
                out << ")";
            },
            TINT_ICE_ON_NO_MATCH
        );
    }
}

void MiniPrinter::EmitExpression(
    std::stringstream& out,
    const tint::ast::Expression* expr,
    OperatorPosition position,
    OperatorGroup parent
) {
    Switch(
        expr,
        [&](const tint::ast::IndexAccessorExpression* a) { EmitIndexAccessor(out, a); },
        [&](const tint::ast::BinaryExpression* b) { EmitBinary(out, b, position, parent); },
        [&](const tint::ast::CallExpression* c) { EmitCall(out, c); },
        [&](const tint::ast::IdentifierExpression* i) { EmitIdentifier(out, i); },
        [&](const tint::ast::LiteralExpression* l) { EmitLiteral(out, l); },
        [&](const tint::ast::MemberAccessorExpression* m) { EmitMemberAccessor(out, m); },
        [&](const tint::ast::PhonyExpression*) { out << "_"; },
        [&](const tint::ast::UnaryOpExpression* u) { EmitUnaryOp(out, u, position, parent); },
        TINT_ICE_ON_NO_MATCH
    );
}

void MiniPrinter::EmitIndexAccessor(std::stringstream& out, const tint::ast::IndexAccessorExpression* expr) {
    bool paren_lhs =
        !expr->object
             ->IsAnyOf<tint::ast::AccessorExpression, tint::ast::CallExpression, tint::ast::IdentifierExpression>();
    if (paren_lhs) {
        out << "(";
    }
    EmitExpression(out, expr->object, OperatorPosition::Left, OperatorGroup::None);
    if (paren_lhs) {
        out << ")";
    }
    out << "[";
    EmitExpression(out, expr->index, OperatorPosition::Left, OperatorGroup::None);
    out << "]";
}

void MiniPrinter::EmitMemberAccessor(std::stringstream& out, const tint::ast::MemberAccessorExpression* expr) {
    bool paren_lhs =
        !expr->object
             ->IsAnyOf<tint::ast::AccessorExpression, tint::ast::CallExpression, tint::ast::IdentifierExpression>();
    if (paren_lhs) {
        out << "(";
    }
    EmitExpression(out, expr->object, OperatorPosition::Left, OperatorGroup::None);
    if (paren_lhs) {
        out << ")";
    }
    out << "." << expr->member->symbol.Name();
}

void MiniPrinter::EmitBinary(
    std::stringstream& out,
    const tint::ast::BinaryExpression* expr,
    OperatorPosition position,
    OperatorGroup parent
) {
    auto group = toOperatorGroup(expr->op);
    auto paren = isParenthesisRequired(group, position, parent);
    if (paren) {
        out << "(";
    }
    EmitExpression(out, expr->lhs, OperatorPosition::Left, group);
    EmitBinaryOp(out, expr->op);
    EmitExpression(out, expr->rhs, OperatorPosition::Right, group);
    if (paren) {
        out << ")";
    }
}

void MiniPrinter::EmitBinaryOp(std::stringstream& out, const tint::core::BinaryOp op) {
    switch (op) {
    case tint::core::BinaryOp::kAnd:
        out << "&";
        return;
    case tint::core::BinaryOp::kOr:
        out << "|";
        return;
    case tint::core::BinaryOp::kXor:
        out << "^";
        return;
    case tint::core::BinaryOp::kLogicalAnd:
        out << "&&";
        return;
    case tint::core::BinaryOp::kLogicalOr:
        out << "||";
        return;
    case tint::core::BinaryOp::kEqual:
        out << "==";
        return;
    case tint::core::BinaryOp::kNotEqual:
        out << "!=";
        return;
    case tint::core::BinaryOp::kLessThan:
        out << "<";
        return;
    case tint::core::BinaryOp::kGreaterThan:
        out << ">";
        return;
    case tint::core::BinaryOp::kLessThanEqual:
        out << "<=";
        return;
    case tint::core::BinaryOp::kGreaterThanEqual:
        out << ">=";
        return;
    case tint::core::BinaryOp::kShiftLeft:
        out << "<<";
        return;
    case tint::core::BinaryOp::kShiftRight:
        out << ">>";
        return;
    case tint::core::BinaryOp::kAdd:
        out << "+";
        return;
    case tint::core::BinaryOp::kSubtract:
        out << "-";
        return;
    case tint::core::BinaryOp::kMultiply:
        out << "*";
        return;
    case tint::core::BinaryOp::kDivide:
        out << "/";
        return;
    case tint::core::BinaryOp::kModulo:
        out << "%";
        return;
    }
    TINT_ICE() << "invalid binary op " << op;
}

void MiniPrinter::EmitCall(std::stringstream& out, const tint::ast::CallExpression* expr) {
    EmitExpression(out, expr->target, OperatorPosition::Left, OperatorGroup::Primary);
    out << "(";
    bool first = true;
    const auto& args = expr->args;
    for (auto* arg : args) {
        if (!first) {
            out << ",";
        }
        first = false;
        EmitExpression(out, arg, OperatorPosition::Left, OperatorGroup::None);
    }
    out << ")";
}

void MiniPrinter::EmitIdentifier(std::stringstream& out, const tint::ast::IdentifierExpression* expr) {
    EmitIdentifier(out, expr->identifier);
}

static const std::unordered_map<std::string, std::string> TypeAliases {
    {"vec2<i32>",   "vec2i"  },
    {"vec3<i32>",   "vec3i"  },
    {"vec4<i32>",   "vec4i"  },
    {"vec2<u32>",   "vec2u"  },
    {"vec3<u32>",   "vec3u"  },
    {"vec4<u32>",   "vec4u"  },
    {"vec2<f32>",   "vec2f"  },
    {"vec3<f32>",   "vec3f"  },
    {"vec4<f32>",   "vec4f"  },
    {"vec2<f16>",   "vec2h"  },
    {"vec3<f16>",   "vec3h"  },
    {"vec4<f16>",   "vec4h"  },
    {"mat2x2<f32>", "mat2x2f"},
    {"mat2x3<f32>", "mat2x3f"},
    {"mat2x4<f32>", "mat2x4f"},
    {"mat3x2<f32>", "mat3x2f"},
    {"mat3x3<f32>", "mat3x3f"},
    {"mat3x4<f32>", "mat3x4f"},
    {"mat4x2<f32>", "mat4x2f"},
    {"mat4x3<f32>", "mat4x3f"},
    {"mat4x4<f32>", "mat4x4f"},
    {"mat2x2<f16>", "mat2x2h"},
    {"mat2x3<f16>", "mat2x3h"},
    {"mat2x4<f16>", "mat2x4h"},
    {"mat3x2<f16>", "mat3x2h"},
    {"mat3x3<f16>", "mat3x3h"},
    {"mat3x4<f16>", "mat3x4h"},
    {"mat4x2<f16>", "mat4x2h"},
    {"mat4x3<f16>", "mat4x3h"},
    {"mat4x4<f16>", "mat4x4h"},
};

void MiniPrinter::EmitIdentifier(std::stringstream& out, const tint::ast::Identifier* ident) {
    if (auto* tmpl_ident = ident->As<tint::ast::TemplatedIdentifier>()) {
        EmitAttributes(out, tmpl_ident->attributes);

        std::stringstream ss;
        ss << ident->symbol.Name() << "<";
        for (auto* expr : tmpl_ident->arguments) {
            if (expr != tmpl_ident->arguments.Front()) {
                ss << ",";
            }
            EmitExpression(ss, expr, OperatorPosition::Left, OperatorGroup::None);
        }
        ss << ">";

        std::string name = std::move(ss).str();
        if (options_->use_type_alias) {
            auto iter = TypeAliases.find(name);
            if (iter != TypeAliases.end()) {
                out << iter->second;
            } else {
                out << name;
            }
        } else {
            out << name;
        }
    } else {
        out << ident->symbol.Name();
    }
}

void MiniPrinter::EmitLiteral(std::stringstream& out, const tint::ast::LiteralExpression* lit) {
    Switch(
        lit,
        [&](const tint::ast::BoolLiteralExpression* l) { out << (l->value ? "true" : "false"); },
        [&](const tint::ast::FloatLiteralExpression* l) {
            if (options_->precise_float) {
                if (l->suffix == tint::ast::FloatLiteralExpression::Suffix::kNone || options_->ignore_literal_suffix) {
                    out << tint::strconv::DoubleToBitPreservingString(l->value);
                } else {
                    out << tint::strconv::FloatToBitPreservingString(static_cast<float>(l->value)) << l->suffix;
                }
            } else {
                auto str = std::format("{:.6f}", l->value);
                while (str.ends_with('0')) {
                    str.pop_back();
                }
                if (str.starts_with("0.") && str.size() > 2) {
                    str = str.substr(1);
                }
                out << str;
                if (!options_->ignore_literal_suffix) {
                    out << l->suffix;
                }
            }
        },
        [&](const tint::ast::IntLiteralExpression* l) {
            out << l->value;
            if (!options_->ignore_literal_suffix) {
                out << l->suffix;
            }
        },
        TINT_ICE_ON_NO_MATCH
    );
}

void MiniPrinter::EmitUnaryOp(
    std::stringstream& out,
    const tint::ast::UnaryOpExpression* expr,
    OperatorPosition position,
    OperatorGroup parent
) {
    auto paren = isParenthesisRequired(OperatorGroup::Unary, position, parent);
    if (paren) {
        out << "(";
    }
    switch (expr->op) {
    case tint::core::UnaryOp::kAddressOf:
        out << "&";
        break;
    case tint::core::UnaryOp::kComplement:
        out << "~";
        break;
    case tint::core::UnaryOp::kIndirection:
        out << "*";
        break;
    case tint::core::UnaryOp::kNot:
        out << "!";
        break;
    case tint::core::UnaryOp::kNegation:
        out << "-";
        break;
    }
    EmitExpression(out, expr->expr, OperatorPosition::Right, OperatorGroup::Unary);
    if (paren) {
        out << ")";
    }
}

std::string MiniPrinter::Result() {
    return std::move(ss_).str();
}

}  // namespace wgslx::writer

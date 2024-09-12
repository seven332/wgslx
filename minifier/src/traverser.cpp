#include "traverser.h"

#include <src/tint/lang/wgsl/ast/accessor_expression.h>
#include <src/tint/lang/wgsl/ast/assignment_statement.h>
#include <src/tint/lang/wgsl/ast/binary_expression.h>
#include <src/tint/lang/wgsl/ast/binding_attribute.h>
#include <src/tint/lang/wgsl/ast/blend_src_attribute.h>
#include <src/tint/lang/wgsl/ast/block_statement.h>
#include <src/tint/lang/wgsl/ast/bool_literal_expression.h>
#include <src/tint/lang/wgsl/ast/break_if_statement.h>
#include <src/tint/lang/wgsl/ast/break_statement.h>
#include <src/tint/lang/wgsl/ast/builtin_attribute.h>
#include <src/tint/lang/wgsl/ast/call_expression.h>
#include <src/tint/lang/wgsl/ast/call_statement.h>
#include <src/tint/lang/wgsl/ast/case_statement.h>
#include <src/tint/lang/wgsl/ast/color_attribute.h>
#include <src/tint/lang/wgsl/ast/compound_assignment_statement.h>
#include <src/tint/lang/wgsl/ast/const.h>
#include <src/tint/lang/wgsl/ast/const_assert.h>
#include <src/tint/lang/wgsl/ast/continue_statement.h>
#include <src/tint/lang/wgsl/ast/diagnostic_attribute.h>
#include <src/tint/lang/wgsl/ast/diagnostic_rule_name.h>
#include <src/tint/lang/wgsl/ast/discard_statement.h>
#include <src/tint/lang/wgsl/ast/float_literal_expression.h>
#include <src/tint/lang/wgsl/ast/for_loop_statement.h>
#include <src/tint/lang/wgsl/ast/group_attribute.h>
#include <src/tint/lang/wgsl/ast/id_attribute.h>
#include <src/tint/lang/wgsl/ast/identifier_expression.h>
#include <src/tint/lang/wgsl/ast/if_statement.h>
#include <src/tint/lang/wgsl/ast/increment_decrement_statement.h>
#include <src/tint/lang/wgsl/ast/index_accessor_expression.h>
#include <src/tint/lang/wgsl/ast/input_attachment_index_attribute.h>
#include <src/tint/lang/wgsl/ast/int_literal_expression.h>
#include <src/tint/lang/wgsl/ast/internal_attribute.h>
#include <src/tint/lang/wgsl/ast/interpolate_attribute.h>
#include <src/tint/lang/wgsl/ast/invariant_attribute.h>
#include <src/tint/lang/wgsl/ast/let.h>
#include <src/tint/lang/wgsl/ast/literal_expression.h>
#include <src/tint/lang/wgsl/ast/location_attribute.h>
#include <src/tint/lang/wgsl/ast/loop_statement.h>
#include <src/tint/lang/wgsl/ast/member_accessor_expression.h>
#include <src/tint/lang/wgsl/ast/must_use_attribute.h>
#include <src/tint/lang/wgsl/ast/override.h>
#include <src/tint/lang/wgsl/ast/parameter.h>
#include <src/tint/lang/wgsl/ast/phony_expression.h>
#include <src/tint/lang/wgsl/ast/return_statement.h>
#include <src/tint/lang/wgsl/ast/stage_attribute.h>
#include <src/tint/lang/wgsl/ast/stride_attribute.h>
#include <src/tint/lang/wgsl/ast/struct_member_align_attribute.h>
#include <src/tint/lang/wgsl/ast/struct_member_offset_attribute.h>
#include <src/tint/lang/wgsl/ast/struct_member_size_attribute.h>
#include <src/tint/lang/wgsl/ast/switch_statement.h>
#include <src/tint/lang/wgsl/ast/unary_op_expression.h>
#include <src/tint/lang/wgsl/ast/var.h>
#include <src/tint/lang/wgsl/ast/variable_decl_statement.h>
#include <src/tint/lang/wgsl/ast/while_statement.h>
#include <src/tint/lang/wgsl/ast/workgroup_attribute.h>
#include <src/tint/utils/rtti/switch.h>

namespace wgslx::minifier {

void Traverse(const tint::ast::Statement* stmt, const std::function<void(const tint::ast::Identifier*)>& block) {
    if (!stmt) {
        return;
    }

    Switch(
        stmt,
        [&](const tint::ast::AssignmentStatement* a) {
            Traverse(a->lhs, block);
            Traverse(a->rhs, block);
        },
        [&](const tint::ast::BlockStatement* b) {
            for (const auto* s : b->statements) {
                Traverse(s, block);
            }
            for (const auto* a : b->attributes) {
                Traverse(a, block);
            }
        },
        [&](const tint::ast::BreakIfStatement* b) { Traverse(b->condition, block); },
        [&](const tint::ast::BreakStatement*) {},
        [&](const tint::ast::CallStatement* c) { Traverse(c->expr, block); },
        [&](const tint::ast::CaseStatement* c) {
            for (const auto* s : c->selectors) {
                Traverse(s->expr, block);
            }
            Traverse(c->body, block);
        },
        [&](const tint::ast::CompoundAssignmentStatement* a) {
            Traverse(a->lhs, block);
            Traverse(a->rhs, block);
        },
        [&](const tint::ast::ConstAssert* a) { Traverse(a->condition, block); },
        [&](const tint::ast::ContinueStatement*) {},
        [&](const tint::ast::DiscardStatement*) {},
        [&](const tint::ast::ForLoopStatement* l) {
            Traverse(l->initializer, block);
            Traverse(l->condition, block);
            Traverse(l->continuing, block);
            Traverse(l->body, block);
            for (const auto* a : l->attributes) {
                Traverse(a, block);
            }
        },
        [&](const tint::ast::IfStatement* i) {
            Traverse(i->condition, block);
            Traverse(i->body, block);
            Traverse(i->else_statement, block);
            for (const auto* a : i->attributes) {
                Traverse(a, block);
            }
        },
        [&](const tint::ast::IncrementDecrementStatement* i) { Traverse(i->lhs, block); },
        [&](const tint::ast::LoopStatement* l) {
            Traverse(l->body, block);
            Traverse(l->continuing, block);
            for (const auto* a : l->attributes) {
                Traverse(a, block);
            }
        },
        [&](const tint::ast::ReturnStatement* r) { Traverse(r->value, block); },
        [&](const tint::ast::SwitchStatement* s) {
            Traverse(s->condition, block);
            for (const auto* c : s->body) {
                Traverse(c, block);
            }
            for (const auto* a : s->attributes) {
                Traverse(a, block);
            }
            for (const auto* a : s->body_attributes) {
                Traverse(a, block);
            }
        },
        [&](const tint::ast::VariableDeclStatement* v) { Traverse(v->variable, block); },
        [&](const tint::ast::WhileStatement* w) {
            Traverse(w->condition, block);
            Traverse(w->body, block);
            for (const auto* a : w->attributes) {
                Traverse(a, block);
            }
        },
        TINT_ICE_ON_NO_MATCH
    );
}

void Traverse(const tint::ast::Expression* expr, const std::function<void(const tint::ast::Identifier*)>& block) {
    if (!expr) {
        return;
    }

    Switch(
        expr,
        [&](const tint::ast::BinaryExpression* b) {
            Traverse(b->lhs, block);
            Traverse(b->rhs, block);
        },
        [&](const tint::ast::CallExpression* c) {
            Traverse(c->target, block);
            for (const auto* a : c->args) {
                Traverse(a, block);
            }
        },
        [&](const tint::ast::IdentifierExpression* i) { block(i->identifier); },
        [&](const tint::ast::PhonyExpression*) {},
        [&](const tint::ast::UnaryOpExpression* o) { Traverse(o->expr, block); },
        [&](const tint::ast::IndexAccessorExpression* a) {
            Traverse(a->object, block);
            Traverse(a->index, block);
        },
        [&](const tint::ast::MemberAccessorExpression* a) {
            Traverse(a->object, block);
            block(a->member);
        },
        [&](const tint::ast::BoolLiteralExpression*) {},
        [&](const tint::ast::IntLiteralExpression*) {},
        [&](const tint::ast::FloatLiteralExpression*) {},
        TINT_ICE_ON_NO_MATCH
    );
}

void Traverse(const tint::ast::Attribute* attr, const std::function<void(const tint::ast::Identifier*)>& block) {
    if (!attr) {
        return;
    }

    Switch(
        attr,
        [&](const tint::ast::BindingAttribute* b) { Traverse(b->expr, block); },
        [&](const tint::ast::BlendSrcAttribute* b) { Traverse(b->expr, block); },
        [&](const tint::ast::BuiltinAttribute*) {},
        [&](const tint::ast::ColorAttribute* c) { Traverse(c->expr, block); },
        [&](const tint::ast::DiagnosticAttribute* d) {
            if (d->control.rule_name->category) {
                block(d->control.rule_name->category);
            }
            block(d->control.rule_name->name);
        },
        [&](const tint::ast::GroupAttribute* g) { Traverse(g->expr, block); },
        [&](const tint::ast::IdAttribute* i) { Traverse(i->expr, block); },
        [&](const tint::ast::InputAttachmentIndexAttribute* i) { Traverse(i->expr, block); },
        [&](const tint::ast::InternalAttribute*) {
            // Skip
        },
        [&](const tint::ast::InterpolateAttribute*) {},
        [&](const tint::ast::InvariantAttribute*) {},
        [&](const tint::ast::LocationAttribute* l) { Traverse(l->expr, block); },
        [&](const tint::ast::MustUseAttribute*) {},
        [&](const tint::ast::StageAttribute*) {},
        [&](const tint::ast::StrideAttribute*) {},
        [&](const tint::ast::StructMemberAlignAttribute* a) { Traverse(a->expr, block); },
        [&](const tint::ast::StructMemberOffsetAttribute* o) { Traverse(o->expr, block); },
        [&](const tint::ast::StructMemberSizeAttribute* s) { Traverse(s->expr, block); },
        [&](const tint::ast::WorkgroupAttribute* w) {
            Traverse(w->x, block);
            Traverse(w->y, block);
            Traverse(w->z, block);
        },
        TINT_ICE_ON_NO_MATCH
    );
}

void Traverse(const tint::ast::Variable* var, const std::function<void(const tint::ast::Identifier*)>& block) {
    if (!var) {
        return;
    }

    Traverse(var->type.expr, block);
    block(var->name);
    Traverse(var->initializer, block);
    for (const auto* a : var->attributes) {
        Traverse(a, block);
    }

    Switch(
        var,
        [&](const tint::ast::Const*) {},
        [&](const tint::ast::Let*) {},
        [&](const tint::ast::Override*) {},
        [&](const tint::ast::Parameter*) {},
        [&](const tint::ast::Var* v) {
            Traverse(v->declared_address_space, block);
            Traverse(v->declared_access, block);
        },
        TINT_ICE_ON_NO_MATCH
    );
}

}  // namespace wgslx::minifier

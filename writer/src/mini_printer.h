#pragma once

#include <src/tint/lang/wgsl/ast/assignment_statement.h>
#include <src/tint/lang/wgsl/ast/binary_expression.h>
#include <src/tint/lang/wgsl/ast/break_if_statement.h>
#include <src/tint/lang/wgsl/ast/compound_assignment_statement.h>
#include <src/tint/lang/wgsl/ast/const_assert.h>
#include <src/tint/lang/wgsl/ast/diagnostic_control.h>
#include <src/tint/lang/wgsl/ast/function.h>
#include <src/tint/lang/wgsl/ast/if_statement.h>
#include <src/tint/lang/wgsl/ast/increment_decrement_statement.h>
#include <src/tint/lang/wgsl/ast/index_accessor_expression.h>
#include <src/tint/lang/wgsl/ast/literal_expression.h>
#include <src/tint/lang/wgsl/ast/loop_statement.h>
#include <src/tint/lang/wgsl/ast/member_accessor_expression.h>
#include <src/tint/lang/wgsl/ast/return_statement.h>
#include <src/tint/lang/wgsl/ast/statement.h>
#include <src/tint/lang/wgsl/ast/switch_statement.h>
#include <src/tint/lang/wgsl/ast/type_decl.h>
#include <src/tint/lang/wgsl/ast/variable.h>
#include <src/tint/lang/wgsl/program/program.h>
#include <src/tint/utils/generator/text_generator.h>

#include <sstream>
#include <string>

#include "operator_group.h"
#include "writer/writer.h"

namespace wgslx::writer {

class MiniPrinter {
 public:
    MiniPrinter(const tint::Program* program, const Options* options) : program_(program), options_(options) {}

    bool Generate();

    std::string Result();

 private:
    const tint::Program* program_;
    const Options* options_;
    std::stringstream ss_;

    void EmitEnables();
    void EmitRequires();
    void EmitDiagnosticDirectives();
    void EmitDiagnosticControl(const tint::ast::DiagnosticControl& diagnostic);
    void EmitTypeDecl(const tint::ast::TypeDecl* td);
    void EmitFunction(const tint::ast::Function* func);
    void EmitStatement(const tint::ast::Statement* stmt);
    void EmitAssign(const tint::ast::AssignmentStatement* stmt);
    void EmitBlock(const tint::ast::BlockStatement* stmt);
    void EmitBreakIf(const tint::ast::BreakIfStatement* b);
    void EmitCompoundAssign(const tint::ast::CompoundAssignmentStatement* stmt);
    void EmitIf(const tint::ast::IfStatement* stmt);
    void EmitIncrementDecrement(const tint::ast::IncrementDecrementStatement* stmt);
    void EmitLoop(const tint::ast::LoopStatement* stmt);
    void EmitForLoop(const tint::ast::ForLoopStatement* stmt);
    void EmitWhile(const tint::ast::WhileStatement* stmt);
    void EmitReturn(const tint::ast::ReturnStatement* stmt);
    void EmitSwitch(const tint::ast::SwitchStatement* stmt);
    void EmitCase(const tint::ast::CaseStatement* stmt);
    void EmitVariable(const tint::ast::Variable* var);
    void EmitConstAssert(const tint::ast::ConstAssert* ca);
    void EmitStructType(const tint::ast::Struct* str);
    void EmitAttributes(tint::VectorRef<const tint::ast::Attribute*> attrs);
    void EmitExpression(const tint::ast::Expression* expr, OperatorPosition position, OperatorGroup parent);
    void EmitIndexAccessor(const tint::ast::IndexAccessorExpression* expr);
    void EmitMemberAccessor(const tint::ast::MemberAccessorExpression* expr);
    void EmitBinary(const tint::ast::BinaryExpression* expr, OperatorPosition position, OperatorGroup parent);
    void EmitBinaryOp(const tint::core::BinaryOp op);
    void EmitCall(const tint::ast::CallExpression* expr);
    void EmitIdentifier(const tint::ast::IdentifierExpression* expr);
    void EmitIdentifier(const tint::ast::Identifier* ident);
    void EmitLiteral(const tint::ast::LiteralExpression* lit);
    void EmitUnaryOp(const tint::ast::UnaryOpExpression* expr, OperatorPosition position, OperatorGroup parent);
};

}  // namespace wgslx::writer

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

    void EmitEnables(std::stringstream& out);
    void EmitRequires(std::stringstream& out);
    void EmitDiagnosticDirectives(std::stringstream& out);
    void EmitDiagnosticControl(std::stringstream& out, const tint::ast::DiagnosticControl& diagnostic);
    void EmitTypeDecl(std::stringstream& out, const tint::ast::TypeDecl* td);
    void EmitFunction(std::stringstream& out, const tint::ast::Function* func);
    void EmitStatement(std::stringstream& out, const tint::ast::Statement* stmt);
    void EmitAssign(std::stringstream& out, const tint::ast::AssignmentStatement* stmt);
    void EmitBlock(std::stringstream& out, const tint::ast::BlockStatement* stmt);
    void EmitBreakIf(std::stringstream& out, const tint::ast::BreakIfStatement* b);
    void EmitCompoundAssign(std::stringstream& out, const tint::ast::CompoundAssignmentStatement* stmt);
    void EmitIf(std::stringstream& out, const tint::ast::IfStatement* stmt);
    void EmitIncrementDecrement(std::stringstream& out, const tint::ast::IncrementDecrementStatement* stmt);
    void EmitLoop(std::stringstream& out, const tint::ast::LoopStatement* stmt);
    void EmitForLoop(std::stringstream& out, const tint::ast::ForLoopStatement* stmt);
    void EmitWhile(std::stringstream& out, const tint::ast::WhileStatement* stmt);
    void EmitReturn(std::stringstream& out, const tint::ast::ReturnStatement* stmt);
    void EmitSwitch(std::stringstream& out, const tint::ast::SwitchStatement* stmt);
    void EmitCase(std::stringstream& out, const tint::ast::CaseStatement* stmt);
    void EmitVariable(std::stringstream& out, const tint::ast::Variable* var);
    void EmitConstAssert(std::stringstream& out, const tint::ast::ConstAssert* ca);
    void EmitStructType(std::stringstream& out, const tint::ast::Struct* str);
    void EmitAttributes(std::stringstream& out, const tint::VectorRef<const tint::ast::Attribute*>& attrs);
    void EmitExpression(
        std::stringstream& out,
        const tint::ast::Expression* expr,
        OperatorPosition position,
        OperatorGroup parent
    );
    void EmitIndexAccessor(std::stringstream& out, const tint::ast::IndexAccessorExpression* expr);
    void EmitMemberAccessor(std::stringstream& out, const tint::ast::MemberAccessorExpression* expr);
    void EmitBinary(
        std::stringstream& out,
        const tint::ast::BinaryExpression* expr,
        OperatorPosition position,
        OperatorGroup parent
    );
    void EmitBinaryOp(std::stringstream& out, const tint::core::BinaryOp op);
    void EmitCall(std::stringstream& out, const tint::ast::CallExpression* expr);
    void EmitIdentifier(std::stringstream& out, const tint::ast::IdentifierExpression* expr);
    void EmitIdentifier(std::stringstream& out, const tint::ast::Identifier* ident);
    void EmitLiteral(std::stringstream& out, const tint::ast::LiteralExpression* lit);
    void EmitUnaryOp(
        std::stringstream& out,
        const tint::ast::UnaryOpExpression* expr,
        OperatorPosition position,
        OperatorGroup parent
    );
};

}  // namespace wgslx::writer

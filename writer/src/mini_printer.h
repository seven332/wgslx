#pragma once

#include <src/tint/lang/wgsl/ast/binary_expression.h>
#include <src/tint/lang/wgsl/ast/const_assert.h>
#include <src/tint/lang/wgsl/ast/diagnostic_control.h>
#include <src/tint/lang/wgsl/ast/function.h>
#include <src/tint/lang/wgsl/ast/index_accessor_expression.h>
#include <src/tint/lang/wgsl/ast/literal_expression.h>
#include <src/tint/lang/wgsl/ast/member_accessor_expression.h>
#include <src/tint/lang/wgsl/ast/type_decl.h>
#include <src/tint/lang/wgsl/ast/variable.h>
#include <src/tint/lang/wgsl/program/program.h>
#include <src/tint/utils/generator/text_generator.h>

#include <sstream>
#include <string>

#include "operator_group.h"

namespace wgslx::writer {

class MiniPrinter {
 public:
    explicit MiniPrinter(const tint::Program* program) : program_(program) {}

    bool Generate();

    std::string Result();

 private:
    const tint::Program* program_;
    std::stringstream ss_;

    void EmitEnables();
    void EmitRequires();
    void EmitDiagnosticDirectives();
    void EmitDiagnosticControl(const tint::ast::DiagnosticControl& diagnostic);
    void EmitTypeDecl(const tint::ast::TypeDecl* td);
    void EmitFunction(const tint::ast::Function* func);
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

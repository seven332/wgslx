#pragma once

#include <src/tint/lang/wgsl/ast/attribute.h>
#include <src/tint/lang/wgsl/ast/case_selector.h>
#include <src/tint/lang/wgsl/ast/expression.h>
#include <src/tint/lang/wgsl/ast/identifier.h>
#include <src/tint/lang/wgsl/ast/statement.h>
#include <src/tint/lang/wgsl/ast/variable.h>
#include <src/tint/utils/symbol/symbol.h>

#include <functional>

namespace wgslx::minifier {

void Traverse(const tint::ast::Statement* stmt, const std::function<void(const tint::ast::Identifier*)>& block);
void Traverse(const tint::ast::Expression* expr, const std::function<void(const tint::ast::Identifier*)>& block);
void Traverse(const tint::ast::Attribute* attr, const std::function<void(const tint::ast::Identifier*)>& block);
void Traverse(const tint::ast::Variable* var, const std::function<void(const tint::ast::Identifier*)>& block);

}  // namespace wgslx::minifier

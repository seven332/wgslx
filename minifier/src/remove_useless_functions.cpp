#include "remove_useless_functions.h"

#include <src/tint/lang/wgsl/ast/function.h>
#include <src/tint/lang/wgsl/ast/identifier.h>
#include <src/tint/lang/wgsl/ast/traverse_expressions.h>
#include <src/tint/lang/wgsl/program/clone_context.h>
#include <src/tint/lang/wgsl/program/program_builder.h>
#include <src/tint/lang/wgsl/resolver/resolve.h>
#include <src/tint/utils/symbol/symbol.h>

#include <iostream>
#include <range/v3/range/conversion.hpp>
#include <range/v3/view/filter.hpp>
#include <range/v3/view/transform.hpp>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "traverser.h"

TINT_INSTANTIATE_TYPEINFO(wgslx::minifier::RemoveUselessFunctions);

namespace wgslx::minifier {

struct FuncRef {
    std::unordered_set<tint::Symbol> funcs;
    const tint::ast::Function* func;
    bool visited = false;
};

using FuncRefs = std::unordered_map<tint::Symbol, FuncRef>;

static FuncRefs CollectFuncRefs(const tint::Program& program) {
    auto refs = program.AST().Functions() | ranges::views::transform([](const tint::ast::Function* function) {
                    return std::make_pair(function->name->symbol, FuncRef {.func = function});
                }) |
                ranges::to<FuncRefs>();

    for (const auto* func : program.AST().Functions()) {
        auto& funcs = refs[func->name->symbol].funcs;
        Traverse(func->body, [&](const tint::ast::Identifier* i) {
            if (refs.contains(i->symbol)) {
                funcs.emplace(i->symbol);
            }
        });
    }

    return refs;
}

static void MarkVisited(FuncRefs& refs, const tint::Symbol& symbol) {
    auto& ref = refs[symbol];
    if (ref.visited) {
        return;
    }
    ref.visited = true;

    for (const auto& s : ref.funcs) {
        MarkVisited(refs, s);
    }
}

static std::vector<const tint::ast::Function*> FindUselessFunctions(const tint::Program& program) {
    auto refs = CollectFuncRefs(program);
    for (const auto& [symbol, ref] : refs) {
        if (ref.func->IsEntryPoint()) {
            MarkVisited(refs, symbol);
        }
    }
    return refs | ranges::views::filter([](const auto& p) { return !p.second.visited; }) |
           ranges::views::transform([](const auto& p) { return p.second.func; }) | ranges::to<std::vector>();
}

RemoveUselessFunctions::ApplyResult RemoveUselessFunctions::Apply(
    const tint::Program& program,
    const tint::ast::transform::DataMap& inputs,
    tint::ast::transform::DataMap& outputs
) const {
    tint::ProgramBuilder builder;
    tint::program::CloneContext ctx(&builder, &program, true);
    for (const auto* func : FindUselessFunctions(*ctx.src)) {
        ctx.Remove(ctx.src->AST().GlobalDeclarations(), func);
    }
    ctx.Clone();
    return tint::resolver::Resolve(builder);
}

}  // namespace wgslx::minifier

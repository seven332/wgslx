#include "remove_useless.h"

#include <src/tint/lang/wgsl/ast/const.h>
#include <src/tint/lang/wgsl/ast/function.h>
#include <src/tint/lang/wgsl/ast/identifier.h>
#include <src/tint/lang/wgsl/ast/traverse_expressions.h>
#include <src/tint/lang/wgsl/program/clone_context.h>
#include <src/tint/lang/wgsl/program/program_builder.h>
#include <src/tint/lang/wgsl/resolver/resolve.h>
#include <src/tint/utils/symbol/symbol.h>

#include <range/v3/range/conversion.hpp>
#include <range/v3/view/filter.hpp>
#include <range/v3/view/transform.hpp>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

#include "traverser.h"

TINT_INSTANTIATE_TYPEINFO(wgslx::minifier::RemoveUseless);

namespace wgslx::minifier {

template<class... Ts>
struct Match : Ts... {
    using Ts::operator()...;
};

template<class... Ts>
Match(Ts...) -> Match<Ts...>;

template<typename... Ts, typename... Fs>
constexpr decltype(auto) operator|(const std::variant<Ts...>& v, const Match<Fs...>& match) {
    return std::visit(match, v);
}

class Entity {
 public:
    explicit Entity(const tint::ast::Function* ptr) : ptr_(ptr) {}
    explicit Entity(const tint::ast::Const* ptr) : ptr_(ptr) {}

    [[nodiscard]] tint::Symbol Symbol() const {
        return ptr_ | Match {
                          [](const tint::ast::Function* f) { return f->name->symbol; },
                          [](const tint::ast::Const* c) { return c->name->symbol; },
                      };
    }

    [[nodiscard]] const tint::ast::Node* Ptr() const {
        return ptr_ | Match {
                          [](const tint::ast::Function* f) -> const tint::ast::Node* { return f; },
                          [](const tint::ast::Const* c) -> const tint::ast::Node* { return c; },
                      };
    }

    [[nodiscard]] bool IsEntryPoint() const {
        return ptr_ | Match {
                          [](const tint::ast::Function* f) { return f->IsEntryPoint(); },
                          [](const tint::ast::Const* /* c */) { return false; },
                      };
    }

 private:
    std::variant<const tint::ast::Function*, const tint::ast::Const*> ptr_;
};

struct Element {
    Entity self;
    std::unordered_set<tint::Symbol> refs;
    bool visited = false;
};

using Elements = std::unordered_map<tint::Symbol, Element>;

static Elements CollectElements(const tint::Program& program) {
    auto elements = program.AST().GlobalDeclarations() | ranges::views::filter([](const tint::ast::Node* node) {
                        return node->Is<tint::ast::Function>() || node->Is<tint::ast::Const>();
                    }) |
                    ranges::views::transform([](const tint::ast::Node* node) {
                        if (node->Is<tint::ast::Function>()) {
                            const auto* function = node->As<tint::ast::Function>();
                            return std::make_pair(function->name->symbol, Element {.self = Entity(function)});
                        } else {
                            const auto* c = node->As<tint::ast::Const>();
                            return std::make_pair(c->name->symbol, Element {.self = Entity(c)});
                        }
                    }) |
                    ranges::to<Elements>();

    for (const auto* node : program.AST().GlobalDeclarations()) {
        tint::Symbol symbol;
        const tint::ast::Statement* statement = nullptr;
        const tint::ast::Expression* expression = nullptr;
        if (node->Is<tint::ast::Function>()) {
            const auto* function = node->As<tint::ast::Function>();
            symbol = function->name->symbol;
            statement = function->body;
        } else if (node->Is<tint::ast::Const>()) {
            const auto* c = node->As<tint::ast::Const>();
            symbol = c->name->symbol;
            expression = c->initializer;
        } else {
            continue;
        }

        auto iter = elements.find(symbol);
        if (iter == elements.end()) {
            continue;
        }

        auto& refs = iter->second.refs;
        Traverse(statement, [&](const tint::ast::Identifier* i) {
            if (elements.contains(i->symbol)) {
                refs.emplace(i->symbol);
            }
        });
        Traverse(expression, [&](const tint::ast::Identifier* i) {
            if (elements.contains(i->symbol)) {
                refs.emplace(i->symbol);
            }
        });
    }

    return elements;
}

static void MarkVisited(Elements& elements, const tint::Symbol& symbol) {
    auto iter = elements.find(symbol);
    if (iter == elements.end()) {
        return;
    }

    if (iter->second.visited) {
        return;
    }
    iter->second.visited = true;

    for (const auto& s : iter->second.refs) {
        MarkVisited(elements, s);
    }
}

static std::vector<const tint::ast::Node*> FindUseless(const tint::Program& program) {
    auto elements = CollectElements(program);
    for (const auto& [symbol, element] : elements) {
        if (element.self.IsEntryPoint()) {
            MarkVisited(elements, symbol);
        }
    }
    return elements | ranges::views::filter([](const auto& p) { return !p.second.visited; }) |
           ranges::views::transform([](const auto& p) { return p.second.self.Ptr(); }) | ranges::to<std::vector>();
}

RemoveUseless::ApplyResult RemoveUseless::Apply(
    const tint::Program& program,
    const tint::ast::transform::DataMap& /* inputs */,
    tint::ast::transform::DataMap& /* outputs */
) const {
    tint::ProgramBuilder builder;
    tint::program::CloneContext ctx(&builder, &program, true);
    for (const auto* node : FindUseless(*ctx.src)) {
        ctx.Remove(ctx.src->AST().GlobalDeclarations(), node);
    }
    ctx.Clone();
    return tint::resolver::Resolve(builder);
}

}  // namespace wgslx::minifier

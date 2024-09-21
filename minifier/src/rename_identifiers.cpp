#include "rename_identifiers.h"

#include <src/tint/lang/core/builtin_fn.h>
#include <src/tint/lang/core/builtin_type.h>
#include <src/tint/lang/wgsl/ast/identifier.h>
#include <src/tint/lang/wgsl/ast/module.h>
#include <src/tint/lang/wgsl/ast/type_decl.h>
#include <src/tint/lang/wgsl/program/clone_context.h>
#include <src/tint/lang/wgsl/program/program_builder.h>
#include <src/tint/lang/wgsl/resolver/resolve.h>
#include <src/tint/lang/wgsl/sem/builtin_enum_expression.h>
#include <src/tint/lang/wgsl/sem/builtin_fn.h>
#include <src/tint/lang/wgsl/sem/call.h>
#include <src/tint/lang/wgsl/sem/member_accessor_expression.h>
#include <src/tint/lang/wgsl/sem/type_expression.h>
#include <src/tint/lang/wgsl/sem/value_constructor.h>
#include <src/tint/lang/wgsl/sem/value_conversion.h>
#include <src/tint/utils/rtti/switch.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstring>
#include <iterator>
#include <range/v3/range/conversion.hpp>
#include <range/v3/view/filter.hpp>
#include <range/v3/view/transform.hpp>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

TINT_INSTANTIATE_TYPEINFO(wgslx::minifier::RenameIdentifiers);
TINT_INSTANTIATE_TYPEINFO(wgslx::minifier::RenameIdentifiers::Data);

namespace wgslx::minifier {

// https://www.w3.org/TR/WGSL/#keyword-summary
static constexpr const char* KeywordSummary[] = {
    "alias",    "break",    "case",   "const",  "const_assert", "continue", "continuing", "default", "diagnostic",
    "discard",  "else",     "enable", "false",  "fn",           "for",      "if",         "let",     "loop",
    "override", "requires", "return", "struct", "switch",       "true",     "var",        "while",

};

// https://www.w3.org/TR/WGSL/#reserved-words
static constexpr const char* ReservedWords[] = {
    "NULL",
    "Self",
    "abstract",
    "active",
    "alignas",
    "alignof",
    "as",
    "asm",
    "asm_fragment",
    "async",
    "attribute",
    "auto",
    "await",
    "become",
    "binding_array",
    "cast",
    "catch",
    "class",
    "co_await",
    "co_return",
    "co_yield",
    "coherent",
    "column_major",
    "common",
    "compile",
    "compile_fragment",
    "concept",
    "const_cast",
    "consteval",
    "constexpr",
    "constinit",
    "crate",
    "debugger",
    "decltype",
    "delete",
    "demote",
    "demote_to_helper",
    "do",
    "dynamic_cast",
    "enum",
    "explicit",
    "export",
    "extends",
    "extern",
    "external",
    "fallthrough",
    "filter",
    "final",
    "finally",
    "friend",
    "from",
    "fxgroup",
    "get",
    "goto",
    "groupshared",
    "highp",
    "impl",
    "implements",
    "import",
    "inline",
    "instanceof",
    "interface",
    "layout",
    "lowp",
    "macro",
    "macro_rules",
    "match",
    "mediump",
    "meta",
    "mod",
    "module",
    "move",
    "mut",
    "mutable",
    "namespace",
    "new",
    "nil",
    "noexcept",
    "noinline",
    "nointerpolation",
    "noperspective",
    "null",
    "nullptr",
    "of",
    "operator",
    "package",
    "packoffset",
    "partition",
    "pass",
    "patch",
    "pixelfragment",
    "precise",
    "precision",
    "premerge",
    "priv",
    "protected",
    "pub",
    "public",
    "readonly",
    "ref",
    "regardless",
    "register",
    "reinterpret_cast",
    "require",
    "resource",
    "restrict",
    "self",
    "set",
    "shared",
    "sizeof",
    "smooth",
    "snorm",
    "static",
    "static_assert",
    "static_cast",
    "std",
    "subroutine",
    "super",
    "target",
    "template",
    "this",
    "thread_local",
    "throw",
    "trait",
    "try",
    "type",
    "typedef",
    "typeid",
    "typename",
    "typeof",
    "union",
    "unless",
    "unorm",
    "unsafe",
    "unsized",
    "use",
    "using",
    "varying",
    "virtual",
    "volatile",
    "wgsl",
    "where",
    "with",
    "writeonly",
    "yield",
};

template<class T, std::size_t N>
static constexpr std::size_t ArraySize(T (& /* _ */)[N]) {
    return N;
}

static constexpr auto KeywordSize = ArraySize(KeywordSummary) + ArraySize(ReservedWords) +
                                    ArraySize(tint::core::kBuiltinTypeStrings) +
                                    ArraySize(tint::core::kBuiltinFnStrings);

static constexpr bool Compare(const char* a, const char* b) {
    return __builtin_strcmp(a, b) < 0;
}

static constexpr std::array<const char*, KeywordSize> SortedKeywords = []() {
    std::array<const char*, KeywordSize> keywords;
    auto i = 0;
    for (auto j = 0; j < static_cast<int>(ArraySize(KeywordSummary)); ++i, ++j) {
        keywords[i] = KeywordSummary[j];
    }
    for (auto j = 0; j < static_cast<int>(ArraySize(ReservedWords)); ++i, ++j) {
        keywords[i] = ReservedWords[j];
    }
    for (auto j = 0; j < static_cast<int>(ArraySize(tint::core::kBuiltinTypeStrings)); ++i, ++j) {
        keywords[i] = tint::core::kBuiltinTypeStrings[j].data();
    }
    for (auto j = 0; j < static_cast<int>(ArraySize(tint::core::kBuiltinFnStrings)); ++i, ++j) {
        keywords[i] = tint::core::kBuiltinFnStrings[j];
    }
    std::sort(keywords.begin(), keywords.end(), Compare);
    return keywords;
}();

static bool IsKeyword(const char* str) {
    return std::binary_search(SortedKeywords.begin(), SortedKeywords.end(), str, Compare);
}

static std::string ToName(int index) {
    static constexpr int HalfLowBase = ('z' - 'a' + 1);
    static_assert('Z' - 'A' + 1 == HalfLowBase);
    static constexpr int LowBase = HalfLowBase * 2;
    static constexpr int HighBase = LowBase + ('9' - '0' + 1);

    auto to_char = [](int i) -> char {
        if (i < HalfLowBase) {
            return static_cast<char>('a' + i);
        } else if (i < LowBase) {
            return static_cast<char>('A' + (i - HalfLowBase));
        } else {
            return static_cast<char>('0' + (i - LowBase));
        }
    };

    std::stringstream ss;
    for (auto max = LowBase, size = 0;; max *= HighBase, ++size) {
        if (index < max) {
            for (auto i = 0; i < size; ++i) {
                ss << to_char(index % HighBase);
                index = index / HighBase;
            }
            ss << to_char(index % LowBase);
            index = index / LowBase;
            assert(index == 0);
            break;
        } else {
            index -= max;
        }
    }

    auto str = std::move(ss).str();
    std::reverse(str.begin(), str.end());
    return str;
}

static std::string NextValidName(int& index) {
    std::string name;
    do {
        name = ToName(index);
        ++index;
    } while (IsKeyword(name.c_str()));
    return name;
}

static tint::Hashset<const tint::ast::Identifier*, 16> CollectPreservedIdentifiers(const tint::Program& src) {
    tint::Hashset<tint::Symbol, 16> global_decls;
    for (auto* decl : src.AST().TypeDecls()) {
        global_decls.Add(decl->name->symbol);
    }

    // Identifiers that need to keep their symbols preserved.
    tint::Hashset<const tint::ast::Identifier*, 16> preserved_identifiers;

    for (auto* node : src.ASTNodes().Objects()) {
        auto preserve_if_builtin_type = [&](const tint::ast::Identifier* ident) {
            if (!global_decls.Contains(ident->symbol)) {
                preserved_identifiers.Add(ident);
            }
        };

        Switch(
            node,
            [&](const tint::ast::MemberAccessorExpression* accessor) {
                auto* sem = src.Sem().Get(accessor)->Unwrap();
                if (sem->Is<tint::sem::Swizzle>()) {
                    preserved_identifiers.Add(accessor->member);
                } else if (auto* str_expr = src.Sem().GetVal(accessor->object)) {
                    if (auto* ty = str_expr->Type()->UnwrapPtrOrRef()->As<tint::core::type::Struct>()) {
                        if (!ty->Is<tint::sem::Struct>()) {
                            // Builtin structure
                            preserved_identifiers.Add(accessor->member);
                        }
                    }
                }
            },
            [&](const tint::ast::DiagnosticAttribute* diagnostic) {
                if (auto* category = diagnostic->control.rule_name->category) {
                    preserved_identifiers.Add(category);
                }
                preserved_identifiers.Add(diagnostic->control.rule_name->name);
            },
            [&](const tint::ast::DiagnosticDirective* diagnostic) {
                if (auto* category = diagnostic->control.rule_name->category) {
                    preserved_identifiers.Add(category);
                }
                preserved_identifiers.Add(diagnostic->control.rule_name->name);
            },
            [&](const tint::ast::IdentifierExpression* expr) {
                Switch(
                    src.Sem().Get(expr),
                    [&](const tint::sem::BuiltinEnumExpressionBase*) { preserved_identifiers.Add(expr->identifier); },
                    [&](const tint::sem::TypeExpression*) { preserve_if_builtin_type(expr->identifier); }
                );
            },
            [&](const tint::ast::CallExpression* call) {
                Switch(
                    src.Sem().Get(call)->UnwrapMaterialize()->As<tint::sem::Call>()->Target(),
                    [&](const tint::sem::BuiltinFn*) { preserved_identifiers.Add(call->target->identifier); },
                    [&](const tint::sem::ValueConversion*) { preserve_if_builtin_type(call->target->identifier); },
                    [&](const tint::sem::ValueConstructor*) { preserve_if_builtin_type(call->target->identifier); }
                );
            }
        );
    }

    return preserved_identifiers;
}

RenameIdentifiers::ApplyResult RenameIdentifiers::Apply(
    const tint::Program& program,
    const tint::ast::transform::DataMap& /* inputs */,
    tint::ast::transform::DataMap& outputs
) const {
    auto preserved_identifiers = CollectPreservedIdentifiers(program);

    auto entry_points = program.AST().Functions() |
                        ranges::views::filter([](const tint::ast::Function* f) { return f->IsEntryPoint(); }) |
                        ranges::views::transform([](const tint::ast::Function* f) { return f->name->symbol.Name(); }) |
                        ranges::to<std::unordered_set>();

    tint::Hashmap<tint::Symbol, tint::Symbol, 32> remappings;
    auto nameIndex = 0;

    tint::ProgramBuilder builder;
    tint::program::CloneContext ctx {&builder, &program, false};
    ctx.ReplaceAll([&](const tint::ast::Identifier* ident) -> const tint::ast::Identifier* {
        if (preserved_identifiers.Contains(ident)) {
            // Preserve symbol
            return nullptr;
        }

        const auto& symbol = ident->symbol;

        // Create a replacement for this symbol, if we haven't already.
        auto replacement = remappings.GetOrAdd(symbol, [&] { return builder.Symbols().New(NextValidName(nameIndex)); });

        // Reconstruct the identifier
        if (auto* tmpl_ident = ident->As<tint::ast::TemplatedIdentifier>()) {
            auto args = ctx.Clone(tmpl_ident->arguments);
            return ctx.dst->create<tint::ast::TemplatedIdentifier>(
                ctx.Clone(ident->source),
                replacement,
                std::move(args),
                tint::Empty
            );
        } else {
            return ctx.dst->create<tint::ast::Identifier>(ctx.Clone(ident->source), replacement);
        }
    });
    ctx.Clone();

    Remappings out;
    for (const auto& it : remappings) {
        auto from = it.key->Name();
        if (entry_points.contains(from)) {
            out[from] = it.value.Name();
        }
    }
    outputs.Add<Data>(std::move(out));

    return tint::resolver::Resolve(builder);
}

}  // namespace wgslx::minifier

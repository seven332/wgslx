#include "formatter/formatter.h"

#include <src/tint/lang/wgsl/program/program.h>

#include "mini_printer.h"

namespace wgslx::formatter {

Result Format(const tint::Program& program, const Options& options) {
    MiniPrinter printer(&program);
    printer.Generate();
    return {
        .wgsl = printer.Result(),
    };
}

}  // namespace wgslx::formatter

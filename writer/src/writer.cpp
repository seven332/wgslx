#include "writer/writer.h"

#include <src/tint/lang/wgsl/program/program.h>

#include "mini_printer.h"

namespace wgslx::writer {

Result Write(const tint::Program& program, const Options& options) {
    MiniPrinter printer(&program, &options);
    printer.Generate();
    return {
        .wgsl = printer.Result(),
    };
}

}  // namespace wgslx::writer

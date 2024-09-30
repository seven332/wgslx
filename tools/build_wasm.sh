#!/usr/bin/env bash

set -eu

SOURCE_DIR=$(dirname $0)/..
BUILD_DIR=$(dirname $0)/../out/wasm

emcmake cmake -S $(dirname $0)/../ -B $BUILD_DIR -DCMAKE_BUILD_TYPE=Release -DDAWN_EMSCRIPTEN_TOOLCHAIN=""
emmake make -C $BUILD_DIR -j cmd

#!/usr/bin/env bash
# Compile corpus kernels to clean-SSA LLVM IR, then extract scheduling DAGs.
#
# Requires a Homebrew (or other) LLVM with clang + opt, and the built pass:
#   brew install llvm cmake ninja
#   cmake -S pass -B pass/build -G Ninja -DLLVM_DIR=$(brew --prefix llvm)/lib/cmake/llvm
#   cmake --build pass/build
#
# Usage: ./corpus/build_corpus.sh
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
LLVM="${LLVM:-$(brew --prefix llvm 2>/dev/null || echo /usr/local)}"
CLANG="$LLVM/bin/clang"
OPT="$LLVM/bin/opt"
PLUGIN="$ROOT/pass/build/MirSchedPass.dylib"   # .so on Linux

mkdir -p "$ROOT/corpus/ll" "$ROOT/corpus/dag"

if [[ ! -x "$CLANG" ]]; then
    echo "clang not found at $CLANG (set LLVM=... to your LLVM prefix)" >&2
    exit 1
fi

for src in "$ROOT"/corpus/src/*.c; do
    name="$(basename "${src%.c}")"
    echo "[build_corpus] $name"
    # -O1 gives clean SSA (mem2reg) without heavy transforms.
    "$CLANG" -O1 -S -emit-llvm "$src" -o "$ROOT/corpus/ll/$name.ll"
done

if [[ -f "$PLUGIN" ]]; then
    for ll in "$ROOT"/corpus/ll/*.ll; do
        "$OPT" -load-pass-plugin="$PLUGIN" \
               -passes="extract-dag<dir=$ROOT/corpus/dag>" \
               -disable-output "$ll"
    done
    echo "[build_corpus] DAGs written to corpus/dag/"
else
    echo "[build_corpus] plugin not built ($PLUGIN); skipping DAG extraction." >&2
    echo "               Build pass/ first, or use the sample DAGs in corpus/dag/." >&2
fi

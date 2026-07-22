#!/usr/bin/env python3
"""Day-8 helper: compare your scheduler's instruction order against LLVM's
in-tree MachineScheduler, QUALITATIVELY.

We are comparing two different abstraction levels and cost models, so this is a
sanity/discussion aid, NOT a numeric equivalence. Report Kendall-tau-style
order similarity and eyeball where the decisions diverge.

Produce the reference MIR with:
    $(brew --prefix llvm)/bin/llc -mtriple=nvptx64-nvidia-cuda \
        -stop-after=machine-scheduler corpus/ll/saxpy.ll -o results/saxpy.mir

Usage:
    python harness/compare_llvm.py results/saxpy.mir
"""
import re
import sys


def instruction_order(mir_path):
    """Very rough: list opcodes in the first machine basic block."""
    ops = []
    in_body = False
    with open(mir_path) as f:
        for line in f:
            s = line.strip()
            if s.startswith("bb."):
                in_body = True
                continue
            if in_body:
                m = re.match(r"(?:\$?\w+\s*=\s*)?([A-Za-z_][\w.]+)", s)
                if m and not s.startswith(("body:", "name:", "-")):
                    ops.append(m.group(1))
    return ops


def main():
    if len(sys.argv) < 2:
        sys.exit("usage: compare_llvm.py <file.mir>")
    ops = instruction_order(sys.argv[1])
    print(f"parsed {len(ops)} machine instructions from {sys.argv[1]}")
    for i, op in enumerate(ops):
        print(f"  {i:3d}  {op}")
    print("\nNOTE: compare this ordering to your scheduler's --sched=list output")
    print("qualitatively. Different cost models => do NOT expect identical order.")


if __name__ == "__main__":
    main()

#!/usr/bin/env python3
"""Run the mirsched engine over the DAG corpus and collect results.csv.

This is harness/orchestration code (safe to treat as vibe-coded). The metrics it
records come entirely from the C++ engine you implement.

Usage:
    python harness/run_experiments.py \
        --bin build/mirsched/mirsched \
        --dags corpus/dag \
        --config harness/configs/gpu.json \
        --out results/results.csv
"""
import argparse
import csv
import glob
import os
import subprocess
import sys

POLICIES = ["program", "list", "list-pressure"]
REG_BUDGETS = [8, 16, 32]
HEADER = [
    "function", "policy", "regs", "sched_length",
    "peak_pressure", "spills", "est_occupancy", "valid",
]


def run_one(binary, dag, policy, regs, config):
    cmd = [binary, dag, f"--sched={policy}", f"--regs={regs}", "--csv"]
    if config:
        cmd.append(f"--config={config}")
    out = subprocess.run(cmd, capture_output=True, text=True, check=True)
    return out.stdout.strip()


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--bin", default="build/mirsched/mirsched")
    ap.add_argument("--dags", default="corpus/dag")
    ap.add_argument("--config", default="harness/configs/gpu.json")
    ap.add_argument("--out", default="results/results.csv")
    ap.add_argument("--runs", type=int, default=5,
                    help="repeat count (engine is deterministic; guards I/O)")
    args = ap.parse_args()

    if not os.path.exists(args.bin):
        sys.exit(f"engine binary not found: {args.bin} (build it first)")

    dags = sorted(glob.glob(os.path.join(args.dags, "*.json")))
    if not dags:
        sys.exit(f"no DAGs found under {args.dags}")

    os.makedirs(os.path.dirname(args.out) or ".", exist_ok=True)
    rows = []
    for dag in dags:
        for policy in POLICIES:
            for regs in REG_BUDGETS:
                last = None
                for _ in range(args.runs):
                    last = run_one(args.bin, dag, policy, regs, args.config)
                if last:
                    rows.append(last.split(","))

    with open(args.out, "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(HEADER)
        w.writerows(rows)

    valid = sum(1 for r in rows if r[-1] == "1")
    print(f"wrote {len(rows)} rows to {args.out} ({valid} fully-valid).")
    if valid == 0:
        print("NOTE: no fully-valid rows yet -- implement ListScheduler (Day 4) "
              "and LinearScan (Day 6) to populate the optimized columns.")


if __name__ == "__main__":
    main()

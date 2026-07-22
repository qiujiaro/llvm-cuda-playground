#!/usr/bin/env python3
"""Plot experiment figures from results.csv.

Requires: pandas, matplotlib  (pip install pandas matplotlib)

Usage:
    python harness/plot.py --in results/results.csv --outdir results/figures
"""
import argparse
import os


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--in", dest="infile", default="results/results.csv")
    ap.add_argument("--outdir", default="results/figures")
    args = ap.parse_args()

    try:
        import pandas as pd
        import matplotlib
        matplotlib.use("Agg")
        import matplotlib.pyplot as plt
    except ImportError:
        raise SystemExit("pip install pandas matplotlib to generate figures")

    os.makedirs(args.outdir, exist_ok=True)
    df = pd.read_csv(args.infile)
    df = df[df["sched_length"] != "NA"].copy()
    if df.empty:
        raise SystemExit("no schedulable rows yet -- implement the scheduler first")
    df["sched_length"] = df["sched_length"].astype(float)

    # Figure 1: schedule length by policy, per function (fixed reg budget).
    sub = df[df["regs"] == df["regs"].min()]
    pivot = sub.pivot_table(index="function", columns="policy",
                            values="sched_length", aggfunc="min")
    ax = pivot.plot(kind="bar", figsize=(9, 5))
    ax.set_ylabel("schedule length (model cycles)")
    ax.set_title("Experiment 1: schedule length by policy (cost-model metric)")
    plt.tight_layout()
    fig1 = os.path.join(args.outdir, "exp1_schedule_length.png")
    plt.savefig(fig1, dpi=120)
    print(f"wrote {fig1}")

    # Figure 2: length vs spills trade-off (Experiment 2), if spills present.
    sp = df[df["spills"] != "NA"].copy()
    if not sp.empty:
        sp["spills"] = sp["spills"].astype(float)
        plt.figure(figsize=(7, 5))
        for policy, g in sp.groupby("policy"):
            plt.scatter(g["sched_length"], g["spills"], label=policy, s=40)
        plt.xlabel("schedule length (model cycles)")
        plt.ylabel("spills")
        plt.title("Experiment 2: length vs spills trade-off")
        plt.legend()
        plt.tight_layout()
        fig2 = os.path.join(args.outdir, "exp2_length_vs_spills.png")
        plt.savefig(fig2, dpi=120)
        print(f"wrote {fig2}")
    else:
        print("skipping Fig2 (no spill data yet -- implement LinearScan, Day 6)")


if __name__ == "__main__":
    main()

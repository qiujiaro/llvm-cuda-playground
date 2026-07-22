# mirsched — Experiment Report (TEMPLATE)

> Fill every `TBD` with numbers from real runs. **Do not pre-fill results.**
> All metrics are cost-model values (see [COST_MODEL.md](COST_MODEL.md)); they are
> not real-hardware performance.

## 1. Setup
- LLVM version (`llvm-config --version`): `TBD`  — verified on `TBD`
- Machine / OS: `TBD`
- Corpus: `TBD` kernels (`corpus/src`), extracted via ExtractDAG.
- Cost model config(s): `harness/configs/gpu.json`, `cpu.json`.
- Runs per point: `TBD` (engine is deterministic; repeats guard I/O only).

## 2. Metrics
- **schedule length** — modelled cycles to issue the region.
- **peak register pressure** — max simultaneously-live values.
- **spills** — linear-scan spill count at register budget K.
- **estimated occupancy** — directional, see COST_MODEL.md.

## Experiment 1 — list scheduling vs program order
- Hypothesis: latency-aware list scheduling lowers modelled schedule length,
  most on memory-bound kernels.
- Independent variable: policy (`program` vs `list`). Controlled: corpus, config, K.
- Result table: `TBD`  ·  Figure: `results/figures/exp1_schedule_length.png`
- Conclusions we CAN draw: `TBD`
- Conclusions we CANNOT draw: real GPU wall-clock speedup.

## Experiment 2 — pressure-aware trade-off
- Hypothesis: pressure-aware tie-break lowers peak pressure / spills, possibly
  raising schedule length slightly.
- Variable: `list` vs `list-pressure`. Controlled: corpus, config, K.
- Result: `TBD`  ·  Figure: `results/figures/exp2_length_vs_spills.png`
- CAN draw: `TBD`  ·  CANNOT draw: which is faster on real hardware.

## Experiment 3 — memory-latency sensitivity
- Hypothesis: benefit of scheduling grows with `memLatency`.
- Variable: `memLatency ∈ {TBD}`. Controlled: corpus, policy pair, K.
- Result: `TBD`  ·  Figure: `TBD`
- CAN draw: `TBD`  ·  CANNOT draw: absolute occupancy/throughput.

## (Optional) Experiment 4 — register-budget sweep
- `TBD`

## LLVM MachineScheduler qualitative comparison
- `TBD` (see `harness/compare_llvm.py`). Different abstraction/cost model → order
  differences are expected and discussed, not scored numerically.

## Threats to validity
- Small/regular corpus; conservative memory edges; model omissions (COST_MODEL.md).

## Conclusion
- `TBD` (keep it conservative and scoped to the model).

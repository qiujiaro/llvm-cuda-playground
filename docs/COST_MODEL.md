# Cost Model — assumptions & limits

**Read this before quoting any number.** `mirsched` produces *model* metrics, not
hardware measurements. The cost model is intentionally simple and directional.

## What it models

- **Per-opcode latency** (`CostModel::latencyOf`): loads/stores = `memLatency`,
  div/sqrt/call = `specialLatency`, everything else = `aluLatency`.
- **Issue width**: at most `issueWidth` instructions issue per cycle.
- **Schedule length**: `max over nodes (issueCycle + latency)` — the modelled
  cycle count of the scheduled region.
- **Register pressure**: max simultaneously-live values across the schedule.
- **Estimated occupancy** (`estimateOccupancy`): a *directional* function of peak
  pressure and `regFileSize` — "more registers live ⇒ fewer resident warps".

## What it does NOT model (non-exhaustive)

- Real ISA encoding, dual-issue rules, port/functional-unit contention.
- Memory hierarchy, coalescing, cache/bank conflicts, real latency variance.
- Warp scheduling, control divergence, barriers, real occupancy tables.
- Cross-basic-block / loop-carried effects (scheduling is per-region).
- Register classes / aliasing / calling conventions.

## Honesty rules for the report

1. Always call results "modelled schedule length / pressure / spills".
2. Never claim a % speedup on real GPUs from these numbers.
3. The occupancy figure is an estimate with an arbitrary normalization; present
   it as a *trend*, not a percentage of real occupancy.
4. The optional PTX/GPU experiment (guide §7, Exp 5) is a separate, qualitative
   cross-check — it does not validate the model numerically.

## Parameters (see `harness/configs/`)

| Param | gpu.json | cpu.json | meaning |
|-------|----------|----------|---------|
| `memLatency` | 200 | 4 | load/store latency (cycles) |
| `aluLatency` | 1 | 1 | arithmetic latency |
| `specialLatency` | 8 | 4 | div/sqrt/call latency |
| `issueWidth` | 2 | 4 | issues per cycle |
| `regFileSize` | 65536 | 512 | registers for occupancy model |

# mirsched

A register-pressure-aware **LLVM back-end instruction scheduler & allocator
sandbox**. It extracts an instruction-dependency DAG from real LLVM IR, then runs
a hand-written **list scheduler** and **linear-scan register allocator** under a
configurable SIMT latency/occupancy **cost model**, and reports baseline vs
optimized schedule length, register pressure, spills, and estimated occupancy.

> **Results are cost-model metrics, not real GPU performance.** See
> [docs/COST_MODEL.md](docs/COST_MODEL.md). Real-hardware validation is optional.

Full plan: [`10-Day_Hands-On_Project_Guide.md`](10-Day_Hands-On_Project_Guide.md).
Architecture: [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md).

## What's implemented vs your TODO

| Piece | Status |
|-------|--------|
| DAG load / cost model / live intervals / peak pressure | ✅ provided |
| Program-order **baseline** scheduler | ✅ provided |
| Critical-path DP (`Dag::computeCriticalPath`) | ⬜ **you** (Day 3) |
| List scheduler (`runListCriticalPath`) | ⬜ **you** (Day 4) |
| Pressure-aware tie-break | ⬜ **you** (Day 5) |
| Linear-scan allocation + spills | ⬜ **you** (Day 6) |
| ExtractDAG LLVM pass | ⬜ **you** (Day 1-2, scaffold in `pass/`) |

These TODOs are deliberately left for you — they are the parts you must be able
to explain in an interview and cannot be vibe-coded.

## Build & test the core (no LLVM needed)

```bash
cmake -S engine -B build/mirsched -DCMAKE_BUILD_TYPE=Release
cmake --build build/mirsched --target mirsched mirsched_tests
./build/mirsched/mirsched_tests
./build/mirsched/mirsched corpus/dag/saxpy.json --sched=program --regs=16 --config=harness/configs/gpu.json
```

Run the benchmark harness → CSV → figures:

```bash
python3 harness/run_experiments.py --config harness/configs/gpu.json
python3 harness/plot.py            # needs pandas + matplotlib
```

## Build the LLVM pass (needs Homebrew LLVM)

```bash
brew install llvm cmake ninja
cmake -S pass -B pass/build -G Ninja -DLLVM_DIR=$(brew --prefix llvm)/lib/cmake/llvm
cmake --build pass/build
./corpus/build_corpus.sh          # C -> .ll -> corpus/dag/*.json
```

## Repo layout

```
pass/       ExtractDAG LLVM plugin (only LLVM-dependent part)
engine/     mirsched_core (Dag, CostModel, ListScheduler, LinearScan) + CLI
tests/      zero-dependency unit tests + golden DAGs
corpus/     kernels (src/), IR (ll/), DAGs (dag/), build_corpus.sh
harness/    run_experiments.py, plot.py, compare_llvm.py, configs/
docs/       ARCHITECTURE, COST_MODEL, REPORT
results/    results.csv + figures/ (generated)
```

## Legacy web demo (optional dashboard)

`backend/` (FastAPI), `frontend/` (Next.js), `docker/`, `docker-compose.yml`, and
`engine/{frontend,middleend,backend}` are the original compiler-playground web
app. They're kept as an optional results dashboard and are **not** part of the
`mirsched` core. To run it:

```bash
open -a docker && docker compose up --build   # frontend :3000, backend :8000
cmake -S engine -B build/engine && cmake --build build/engine
./build/engine/llvm-engine
```

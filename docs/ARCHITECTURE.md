# Architecture

`mirsched` is an offline, cost-model-driven LLVM back-end scheduling & allocation
sandbox. It follows the 10-Day guide (`10-Day_Hands-On_Project_Guide.md`).

```mermaid
flowchart TD
    C["corpus/src/*.c"] -->|clang -O1 -emit-llvm| IR["corpus/ll/*.ll"]
    IR -->|opt -load-pass-plugin (pass/ExtractDAG)| DAG["corpus/dag/*.json"]
    DAG --> CORE
    subgraph CORE["engine/ (pure C++, no LLVM dep)"]
      B["Dag + CostModel + critical path"]
      B --> S["ListScheduler (baseline + list)"]
      S --> R["LinearScan + pressure/occupancy"]
    end
    R --> M["metrics"]
    M -->|harness/run_experiments.py| CSV["results/results.csv"]
    CSV -->|harness/plot.py| FIG["results/figures/*.png"]
    IR -->|llc -stop-after=machine-scheduler| MIR["reference MIR"]
    MIR -.qualitative.-> M
```

## Modules

| Dir | Role | LLVM dep | Who implements |
|-----|------|----------|----------------|
| `pass/` | ExtractDAG: IR -> DAG JSON | yes (Homebrew LLVM) | you (Day 1-2) |
| `engine/src/Dag.cpp` | DAG + critical path (Module 1) | no | `computeCriticalPath` = you (Day 3) |
| `engine/src/ListScheduler.cpp` | scheduler (Module 2) | no | baseline provided; `list*` = you (Day 4-5) |
| `engine/src/LinearScan.cpp` | reg alloc (Module 3) | no | intervals/pressure provided; scan loop = you (Day 6) |
| `engine/src/{Json,CostModel}.cpp` | infra + cost model | no | provided scaffolding |
| `harness/` | experiment driver + plots | no | provided; you design experiments |
| `corpus/` | kernels + DAGs | via clang/opt | you extend |
| `tests/` | zero-dep unit tests | no | you add cases as you implement |

## Legacy web demo (optional dashboard)

`engine/frontend|middleend|backend`, `backend/` (FastAPI), `frontend/` (Next.js),
`docker/`, and `docker-compose.yml` are the pre-existing "compiler playground"
web app. They are **not** part of the `mirsched` core; keep them as an optional
visualization/dashboard front for the results, or remove them if you want a
pure back-end-compiler repo. Note the legacy engine only shells out to `clang`
and stubs the rest — the real algorithmic work lives in `mirsched_core`.

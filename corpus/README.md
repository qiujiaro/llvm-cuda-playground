# Corpus

`src/` holds C kernels. `build_corpus.sh` compiles them to `ll/` (clean SSA via `-O1`)
and extracts scheduling DAGs into `dag/` using the ExtractDAG pass.

The `*.json` files currently in `dag/` are **hand-written samples** (marked with
`_note`) so the engine + harness run before you build the LLVM pass. Running
`build_corpus.sh` overwrites them with real ExtractDAG output.

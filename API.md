# IRScope API

IRScope exposes a small HTTP API for compiling C/C++ source code to LLVM IR and inspecting the optimization pipeline.

## Base URL

```text
http://localhost:8000
```

Interactive OpenAPI documentation is also available at:

- Swagger UI: `http://localhost:8000/docs`
- OpenAPI JSON: `http://localhost:8000/openapi.json`

All request and response bodies use JSON. The maximum source size is 20,000 characters.

## Shared compile request

Both `POST /compile` and `POST /profile` accept the same object.

| Field | Type | Required | Default | Allowed values |
| --- | --- | --- | --- | --- |
| `code` | string | Yes | — | Non-empty C/C++ source |
| `language` | string | No | `cpp` | `c`, `cpp` |
| `optimization_level` | string | No | `O2` | `O0`, `O1`, `O2`, `O3`, `Os`, `Oz` |

Example:

```json
{
  "code": "int square(int x) { return x * x; }",
  "language": "cpp",
  "optimization_level": "O2"
}
```

## `GET /health`

Checks whether the API process is reachable.

### Response — `200 OK`

```json
{
  "status": "ok"
}
```

This endpoint only checks the API process. It does not run Clang or LLVM `opt`.

## `POST /compile`

Compiles the submitted source with Clang and returns textual LLVM IR at the requested optimization level.

### Request

```bash
curl -X POST http://localhost:8000/compile \
  -H 'Content-Type: application/json' \
  -d '{
    "code": "int square(int x) { return x * x; }",
    "language": "cpp",
    "optimization_level": "O2"
  }'
```

### Response — `200 OK`

```json
{
  "success": true,
  "llvm_ir": "; ModuleID = ...",
  "compiler_output": "",
  "compile_time_ms": 41.8
}
```

| Field | Type | Description |
| --- | --- | --- |
| `success` | boolean | Always `true` for a successful response |
| `llvm_ir` | string | LLVM IR emitted by Clang |
| `compiler_output` | string | Warnings and diagnostic text written by Clang |
| `compile_time_ms` | number | Clang subprocess wall-clock time in milliseconds |

## `POST /profile`

Generates initial unoptimized IR, runs LLVM's `default<O…>` optimization pipeline, and returns before/after IR with analysis data. Debug line-table information is included in the initial IR so LLVM remarks can refer back to source locations.

### Request

```bash
curl -X POST http://localhost:8000/profile \
  -H 'Content-Type: application/json' \
  -d '{
    "code": "int square(int x) { return x * x; }",
    "language": "cpp",
    "optimization_level": "O2"
  }'
```

### Response — `200 OK`

```json
{
  "compile_time_ms": 41.8,
  "optimization_time_ms": 12.3,
  "before_ir": "; unoptimized LLVM IR ...",
  "after_ir": "; optimized LLVM IR ...",
  "instruction_count_before": 68,
  "instruction_count_after": 41,
  "basic_block_count_before": 9,
  "basic_block_count_after": 6,
  "pass_timings": [
    {
      "name": "SROAPass",
      "time_ms": 0.9
    },
    {
      "name": "InstCombinePass",
      "time_ms": 1.2
    }
  ],
  "remarks": [
    {
      "kind": "passed",
      "pass_name": "loop-vectorize",
      "message": "vectorized loop (vectorization width: 4, interleaved count: 2)",
      "file": "input.cpp",
      "line": 9,
      "column": 5
    }
  ]
}
```

`compile_time_ms` is the complete frontend-and-optimization wall time for this endpoint. `optimization_time_ms` is the wall time spent in LLVM `opt`. Individual `pass_timings` come from LLVM's pass timing instrumentation and may not add up exactly to `optimization_time_ms` because process startup and IR input/output are also included in the latter.

Remark `kind` is one of `passed`, `missed`, `analysis`, or `warning`. Location fields are `null` when LLVM does not emit source location information.

## Errors

Validation errors use FastAPI's standard `422` response. Compiler errors use `400`:

```json
{
  "detail": {
    "message": "Compilation failed.",
    "compiler_output": "input.cpp:1:12: error: expected expression"
  }
}
```

Other possible responses:

| Status | Meaning |
| --- | --- |
| `400` | The submitted source does not compile |
| `408` | Clang or LLVM exceeded the execution timeout |
| `422` | The JSON body or one of its fields is invalid |
| `500` | LLVM optimization failed after source compilation |
| `503` | Clang or LLVM `opt` is not installed or not on `PATH` |

## Frontend integration

The frontend reads `NEXT_PUBLIC_API_URL`, falling back to `http://localhost:8000`. A Run action sends the same request to `/compile` and `/profile` concurrently. `/health` is called when the page loads and controls the backend availability indicator in the header.

The development CORS configuration permits `http://localhost:3000` and `http://127.0.0.1:3000`.

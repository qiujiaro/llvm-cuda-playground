# IRScope Deployment Strategy

## Overview

IRScope is deployed as two independent services:

* **Frontend**: Next.js application hosted on **Vercel**
* **Backend**: FastAPI + LLVM Engine hosted on **Render** using Docker

The frontend communicates with the backend through HTTPS APIs. The backend invokes the native LLVM Engine executable to perform compilation, optimization, and analysis.

```
Browser
    │
    ▼
Vercel (Next.js)
    │
 HTTPS POST /analyze
    ▼
Render (FastAPI)
    │
subprocess
    ▼
compiler_engine (C++)
    │
LLVM / Clang Libraries
    ▼
Analysis Result
    │
JSON
    ▼
Frontend Visualization
```

---

# Repository Structure

```
llvm-cuda-playground/

├── frontend/          # Next.js frontend
├── backend/           # FastAPI REST API
├── engine/            # Native LLVM Engine (C++)
├── docker/
│   └── backend.Dockerfile
├── docker-compose.yml
└── README.md
```

---

# Deployment Workflow

## Step 1 — Build the LLVM Engine

The LLVM Engine is compiled inside the Docker build process using CMake.

The engine produces a standalone executable:

```
compiler_engine
```

This executable performs all LLVM-related work, including:

* Source parsing
* LLVM IR generation
* Optimization pipeline execution
* Static analysis
* JSON output generation

The FastAPI service never directly interacts with LLVM APIs.

---

## Step 2 — Build Backend Docker Image

The backend Docker image uses a multi-stage build.

### Build Stage

Install:

* LLVM
* Clang
* LLVM Development Libraries
* CMake
* Ninja
* Build tools

Compile:

```
compiler_engine
```

---

### Runtime Stage

Install only runtime dependencies:

* Python
* FastAPI
* Uvicorn
* LLVM runtime libraries

Copy the compiled executable into the container:

```
/usr/local/bin/compiler_engine
```

This keeps the production image significantly smaller.

---

## Step 3 — FastAPI Backend

The backend exposes a REST API:

```
POST /analyze
```

When a request arrives:

```
Receive source code
        │
        ▼
subprocess.run(...)
        │
        ▼
compiler_engine
        │
        ▼
JSON
        │
        ▼
Return HTTP response
```

The backend acts only as an orchestration layer.

No compiler logic exists inside FastAPI.

---

## Step 4 — Local Production Validation

Build the production image:

```bash
docker build \
    -f docker/backend.Dockerfile \
    -t irscope-backend .
```

Run locally:

```bash
docker run \
    -p 8000:8000 \
    irscope-backend
```

Verify:

```
GET /health
POST /analyze
```

Ensure the entire pipeline works before deploying.

---

## Step 5 — Deploy Backend

Deploy the Docker image to Render.

Configuration:

* Runtime: Docker
* Dockerfile: `docker/backend.Dockerfile`

Environment Variables:

```
FRONTEND_ORIGIN=https://your-frontend-domain.vercel.app
```

After deployment:

```
https://irscope-api.onrender.com
```

Verify:

```
/health
/docs
/analyze
```

---

## Step 6 — Deploy Frontend

Deploy the Next.js application to Vercel.

Root directory:

```
frontend
```

Environment Variable:

```
NEXT_PUBLIC_API_URL=https://irscope-api.onrender.com
```

After deployment:

```
https://irscope.vercel.app
```

---

## Step 7 — Configure CORS

The backend only accepts requests from the deployed frontend.

Example:

```
https://irscope.vercel.app
```

Avoid using unrestricted CORS (`*`) in production.

---

# Runtime Architecture

```
Browser
    │
    ▼
Frontend (Next.js)
    │
HTTPS
    ▼
FastAPI
    │
subprocess.run()
    ▼
compiler_engine
    │
Compiler.cpp
    │
Optimizer.cpp
    │
Analyzer.cpp
    ▼
AnalysisResult
    │
JSON
    ▼
Frontend
```

---

# Why Use a Standalone LLVM Engine?

Instead of embedding LLVM directly inside Python, the engine is built as an independent executable.

Advantages:

* Fully independent of Python
* Easier debugging
* Easier benchmarking
* Easier profiling
* Cleaner separation of concerns
* No Python ABI compatibility issues
* Can later be reused by other services or languages

The backend becomes a lightweight API gateway rather than a compiler implementation.

---

# Security Considerations

The backend accepts arbitrary source code from users.

To reduce abuse:

* Limit source code size
* Set compilation timeout
* Limit output size
* Rate limit requests
* Run the engine as a non-root user
* Do not execute compiled binaries
* Only generate LLVM IR and perform static analysis

The first public version should **compile and analyze only**. It should never execute user-generated programs.

---

# Future Scaling

The architecture can evolve without changing the frontend.

Current:

```
Frontend
    │
FastAPI
    │
subprocess
    │
compiler_engine
```

Possible future upgrades:

```
Frontend
    │
FastAPI
    │
gRPC
    │
LLVM Engine Service
```

or

```
Frontend
    │
FastAPI
    │
Multiple Engine Workers
```

The standalone engine architecture makes these transitions straightforward while keeping the compiler implementation isolated from the web service.

# Day 1 — Environment, LLVM Pass Plugin, and Back-End Mental Model

> **Date:** 2026-07-22
> **Environment:** macOS arm64 (Apple Silicon)
> **Status:** ✅ Core task completed

## 1. What I accomplished

Today I built and ran a minimal **out-of-tree LLVM New Pass Manager plugin**. The
plugin is compiled as a macOS dynamic library, loaded by `opt`, and executed once
for every function in an LLVM IR file.

Verified output:

```text
[mirsched] function: add
[mirsched] function: sub
```

This does **not** mean I have extracted a SelectionDAG yet. The current
`ExtractDAG.cpp` is a Day-1 scaffolding pass that only reads LLVM IR and prints
function names. Its purpose is to prove that the complete plugin build-and-load
path works before adding more complex compiler logic.

## 2. Where today's work fits in LLVM

The overall compilation path is:

```text
C/C++ source
    ↓ Clang front end
LLVM IR
    ↓ IR optimization passes
opt + MirSchedPass.dylib           ← today's plugin runs here
    ↓ LLVM CodeGen back end
SelectionDAG (SDNode)
    ↓ legalization
Legal target-supported DAG
    ↓ instruction selection
MachineSDNode
    ↓ scheduling and DAG linearization
MachineInstr (mostly virtual registers)
    ↓ register allocation
MachineInstr (physical registers, plus spills/reloads if needed)
    ↓ lowering to the MC layer
MCInst
    ↓ encoding/emission
Assembly or object-code bytes
```

The important distinction is:

* `opt` runs passes over **LLVM IR**.
* SelectionDAG, `MachineInstr`, register allocation, and machine scheduling are
  part of the **CodeGen back end**, normally driven by `llc` or Clang.
* Therefore, a New-PM Function pass loaded into `opt` cannot directly observe
  SelectionDAG merely because its source file is named `ExtractDAG.cpp`.

## 3. Why I read each source

### 3.1 Life of an instruction in LLVM

**Purpose:** build a mental map of the LLVM back end.

The article follows one operation from source code to machine code. The main
lesson is that an “instruction” changes representation several times:

```text
source expression
→ LLVM IR instruction
→ SDNode
→ legalized SDNode
→ MachineSDNode
→ MachineInstr
→ MCInst
→ encoded machine instruction
```

Legalization exists because LLVM IR operations and types are target-independent,
while a real CPU supports only a specific set of operations, types, registers,
and operand constraints.

The article is based on LLVM 3.2, so its high-level model remains useful, but
old commands, source paths, and implementation details should not be copied
without checking the installed LLVM version.

### 3.2 Writing an LLVM Pass (New PM)

**Purpose:** understand the code implemented today.

This document explains:

* what a pass is;
* why a Function pass has a `run(Function &, FunctionAnalysisManager &)` method;
* why the method returns `PreservedAnalyses`;
* how `PassBuilder` associates a pipeline name with a pass;
* how `opt` loads a dynamically linked pass plugin;
* why the plugin exports `llvmGetPassPluginInfo`;
* how `add_llvm_pass_plugin` builds the dynamic library.

The current pass returns:

```cpp
return PreservedAnalyses::all();
```

because it only observes the IR and prints function names; it does not modify
the function or invalidate existing analysis results.

### 3.3 Getting Started with LLVM

**Purpose:** understand the build prerequisites, not learn compiler algorithms.

For this project, the relevant parts are the requirements and supported tool
versions. LLVM itself is installed through Homebrew, so there is no need today
to build the complete LLVM monorepo from source.

The key environment rule is that the following must all come from the same LLVM
installation:

```text
LLVM headers
LLVM libraries
LLVM CMake configuration
llvm-config
opt
```

## 4. The two files I implemented

### `pass/ExtractDAG.cpp`

This file contains both the pass behavior and the runtime registration code.

The behavior:

```cpp
PreservedAnalyses run(Function &F, FunctionAnalysisManager &) {
  errs() << "[mirsched] function: " << F.getName() << "\n";
  return PreservedAnalyses::all();
}
```

For every LLVM IR function, it prints the function's name.

The registration entry point:

```cpp
llvmGetPassPluginInfo()
```

allows `opt` to discover the plugin. Inside its callback, the command-line
pipeline name:

```text
extract-dag
```

is mapped to:

```cpp
ExtractDAG()
```

Despite its filename, the Day-1 version does not extract a DAG. A more literal
name would currently be `FunctionPrinter.cpp`; the existing name represents the
planned direction of the project.

### `pass/CMakeLists.txt`

This file belongs to the **build system**, not to the LLVM compilation pipeline.
It:

1. locates the installed LLVM package;
2. imports LLVM's CMake helpers;
3. compiles `ExtractDAG.cpp`;
4. links it as an LLVM pass plugin;
5. produces `MirSchedPass.dylib` on macOS.

The critical build declaration is:

```cmake
add_llvm_pass_plugin(MirSchedPass ExtractDAG.cpp)
```

## 5. How the complete path runs

There are two separate processes.

### Build time

```text
CMakeLists.txt
    ↓ finds LLVM and configures the target
ExtractDAG.cpp
    ↓ C++ compilation and dynamic linking
pass/build/MirSchedPass.dylib
```

### Run time

```text
/tmp/t.ll
    ↓ parsed by opt into Module / Function / Instruction objects
opt opens MirSchedPass.dylib
    ↓ looks up llvmGetPassPluginInfo
plugin registers its PassBuilder callback
    ↓
-passes=extract-dag selects ExtractDAG
    ↓
FunctionPassManager calls run() for every function
    ↓
function names are printed to stderr
```

The verified command is:

```bash
/opt/homebrew/opt/llvm/bin/opt \
  -load-pass-plugin=pass/build/MirSchedPass.dylib \
  -passes=extract-dag \
  -disable-output \
  /tmp/t.ll
```

`-disable-output` is appropriate because this pass does not transform IR; its
observable result is the diagnostic text printed through `errs()`.

## 6. Reading questions — answered

### Does `MachineInstr` appear before or after register allocation?

It first appears **before** register allocation and continues to exist after it.

* Before register allocation, most operands use virtual registers.
* Register allocation maps virtual registers to physical registers and may add
  spill/reload instructions.
* After register allocation, the program is still represented using
  `MachineInstr`, but now reflects the real register constraints more closely.

### Which symbol lets `opt` discover a New-PM plugin?

```cpp
llvmGetPassPluginInfo
```

The function uses `extern "C"` so the exported symbol has a predictable name
instead of a C++-mangled name.

### Why must an out-of-tree plugin match `opt`'s LLVM version?

LLVM does not promise a stable cross-version C++ API or ABI for pass plugins.
Between LLVM versions, class layouts, headers, function signatures, exported
symbols, plugin interfaces, and internal behavior may change.

A plugin built using one LLVM version and loaded by another version's `opt` can
fail with an incompatible API error, a missing symbol, a linker/loader error, or
a crash. The safest rule is to build the plugin with the headers, libraries,
CMake files, and `llvm-config` belonging to the exact `opt` that will load it.

## 7. Verified environment

```text
llvm-config --version : 22.1.8
brew --prefix llvm    : /opt/homebrew/opt/llvm
host                  : macOS arm64 (Apple Silicon)
plugin artifact       : pass/build/MirSchedPass.dylib
verification date     : 2026-07-22
```

## 8. Debugging notes

| Symptom                                                   | Root cause                                              | Fix                                                                         |
| --------------------------------------------------------- | ------------------------------------------------------- | --------------------------------------------------------------------------- |
| `check_linker_flag: C: needs to be enabled before use`    | LLVM's CMake helpers perform C compiler/linker probes   | Use `project(mirsched_pass LANGUAGES C CXX)`                                |
| `unknown type name 'constexpr'` or errors in LLVM headers | Plugin was compiled with an older C++ language mode     | Set C++17 before creating the target                                        |
| `llvm/IR/Function.h` not found                            | LLVM include directories were not added                 | Add `${LLVM_INCLUDE_DIRS}` and LLVM definitions                             |
| `llvm/Passes/PassPlugin.h` not found                      | LLVM 22 moved the header to `llvm/Plugins/PassPlugin.h` | Use a checked compatibility include for both layouts                        |
| Plugin cannot be loaded                                   | Wrong artifact extension/path or LLVM version mismatch  | Use the Homebrew `opt` and the `.dylib` built against the same installation |

The largest lesson is that out-of-tree LLVM tutorials age quickly. When a
header or API cannot be found, inspect the installed LLVM tree and version
instead of assuming an older tutorial still matches.

The linker warning that a dylib was built for a newer macOS deployment target
was benign in this verified run because the plugin still built, loaded, and
executed successfully.

## 9. Acceptance criteria

* [x] Homebrew LLVM, CMake, and Ninja available
* [x] LLVM version identified as `22.1.8`
* [x] Pass plugin configures and builds with CMake and Ninja
* [x] `MirSchedPass.dylib` is produced
* [x] `opt` loads the plugin
* [x] `-passes=extract-dag` is recognized
* [x] The pass prints both `add` and `sub`
* [ ] Record LLVM version and verification date in the repository README
* [ ] Commit Day-1 files
* [ ] Optionally add `scripts/run_pass.sh`

## 10. What I can explain after Day 1

I can now explain that LLVM uses multiple instruction representations rather
than translating LLVM IR directly into bytes. LLVM IR is lowered into a
target-aware DAG, unsupported operations and types are legalized, instruction
selection chooses target opcodes, scheduling linearizes the DAG into
`MachineInstr`s, register allocation replaces virtual registers with physical
ones, and the MC layer emits assembly or object-code bytes.

I can also explain that today's plugin is intentionally earlier in that system:
it is an out-of-tree LLVM IR pass loaded by `opt`. Its purpose is to validate
the development infrastructure before working with actual CodeGen structures.

## 11. Interview soundbite

> I built an out-of-tree LLVM New-PM Function pass and loaded it dynamically
> into `opt`. That required matching the plugin to the installed LLVM version,
> configuring LLVM's CMake integration, exporting `llvmGetPassPluginInfo`, and
> registering a pipeline parsing callback. The initial pass only inspects LLVM
> IR and prints function names; it does not yet access SelectionDAG, because
> SelectionDAG and `MachineInstr` belong to the CodeGen pipeline rather than
> the `opt` IR pipeline.

## 12. Carry forward

* Determine the correct CodeGen extension point for the project's actual
  scheduling or machine-instruction analysis goal.
* Keep the distinction between an IR pass and a MachineFunction/CodeGen pass
  explicit.
* Rename or clearly document `ExtractDAG.cpp` if it remains an IR-only pass, so
  the source name does not overstate its current behavior.

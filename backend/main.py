import re
import shutil
import subprocess
import tempfile
import time
from pathlib import Path
from typing import Literal

from fastapi import FastAPI, HTTPException
from fastapi.middleware.cors import CORSMiddleware
from pydantic import BaseModel, Field


app = FastAPI(
    title="IRScope API",
    description="Compile C/C++ source to LLVM IR and inspect LLVM optimization passes.",
    version="0.2.0",
)

app.add_middleware(
    CORSMiddleware,
    allow_origins=["http://localhost:3000", "http://127.0.0.1:3000"],
    allow_credentials=True,
    allow_methods=["GET", "POST"],
    allow_headers=["Content-Type"],
)

Language = Literal["c", "cpp"]
OptimizationLevel = Literal["O0", "O1", "O2", "O3", "Os", "Oz"]


class CompileRequest(BaseModel):
    code: str = Field(min_length=1, max_length=20_000, description="C or C++ source code")
    language: Language = Field(default="cpp", description="Source language")
    optimization_level: OptimizationLevel = Field(default="O2", description="LLVM optimization level")


class CompileResponse(BaseModel):
    success: bool
    llvm_ir: str
    compiler_output: str
    compile_time_ms: float


class PassTiming(BaseModel):
    name: str
    time_ms: float


class OptimizationRemark(BaseModel):
    kind: Literal["passed", "missed", "analysis", "warning"]
    pass_name: str
    message: str
    file: str | None = None
    line: int | None = None
    column: int | None = None


class ProfileResponse(BaseModel):
    compile_time_ms: float
    optimization_time_ms: float
    before_ir: str
    after_ir: str
    instruction_count_before: int
    instruction_count_after: int
    basic_block_count_before: int
    basic_block_count_after: int
    pass_timings: list[PassTiming]
    remarks: list[OptimizationRemark]


def _compiler_for(language: Language) -> tuple[str, str]:
    return ("clang", ".c") if language == "c" else ("clang++", ".cpp")


def _find_opt() -> str:
    for executable in ("opt", "opt-20", "opt-19", "opt-18", "opt-17", "opt-16", "opt-15", "opt-14"):
        resolved = shutil.which(executable)
        if resolved:
            return resolved
    raise HTTPException(status_code=503, detail="LLVM opt is not installed or is not available on PATH.")


def _run(command: list[str], timeout: int = 15) -> subprocess.CompletedProcess[str]:
    try:
        return subprocess.run(command, capture_output=True, text=True, timeout=timeout, check=False)
    except FileNotFoundError as error:
        raise HTTPException(status_code=503, detail=f"Required compiler executable '{command[0]}' is unavailable.") from error
    except subprocess.TimeoutExpired as error:
        raise HTTPException(status_code=408, detail="Compilation timed out.") from error


def _raise_compile_error(result: subprocess.CompletedProcess[str]) -> None:
    if result.returncode != 0:
        raise HTTPException(
            status_code=400,
            detail={"message": "Compilation failed.", "compiler_output": result.stderr},
        )


def _ir_metrics(ir: str) -> tuple[int, int]:
    instructions = 0
    basic_blocks = 0
    inside_function = False
    blocks_in_function = 0

    for raw_line in ir.splitlines():
        line = raw_line.strip()
        if line.startswith("define "):
            inside_function = True
            blocks_in_function = 0
            continue
        if not inside_function:
            continue
        if line == "}":
            if blocks_in_function == 0:
                basic_blocks += 1
            inside_function = False
            continue
        if not line or line.startswith((";", "!")):
            continue
        if line.endswith(":") or re.match(r"^[\w.$-]+:\s*(?:;.*)?$", line):
            basic_blocks += 1
            blocks_in_function += 1
            continue
        if not line.startswith(("attributes ", "declare ")):
            instructions += 1

    return instructions, basic_blocks


def _parse_pass_timings(stderr: str) -> list[PassTiming]:
    timings: list[PassTiming] = []
    timing_cell = re.compile(r"([\d.]+)\s*\([^)]*\)")
    for line in stderr.splitlines():
        cells = list(timing_cell.finditer(line))
        if len(cells) < 3:
            continue
        name = line[cells[-1].end():].strip()
        if name in {"Total", "Name"}:
            continue
        timings.append(PassTiming(name=name, time_ms=round(float(cells[-1].group(1)) * 1000, 3)))
    return timings


def _parse_remarks(stderr: str) -> list[OptimizationRemark]:
    remarks: list[OptimizationRemark] = []
    pattern = re.compile(
        r"^(?:(.*?):(\d+):(\d+):\s*)?(remark|warning):\s*(.*?)\s*\[-Rpass(-missed|-analysis)?=([^\]]+)\]$"
    )
    for line in stderr.splitlines():
        match = pattern.match(line.strip())
        if not match:
            continue
        suffix = match.group(6)
        kind: Literal["passed", "missed", "analysis", "warning"]
        if match.group(4) == "warning":
            kind = "warning"
        elif suffix == "-missed":
            kind = "missed"
        elif suffix == "-analysis":
            kind = "analysis"
        else:
            kind = "passed"
        remarks.append(
            OptimizationRemark(
                kind=kind,
                pass_name=match.group(7),
                message=match.group(5),
                file=Path(match.group(1)).name if match.group(1) else None,
                line=int(match.group(2)) if match.group(2) else None,
                column=int(match.group(3)) if match.group(3) else None,
            )
        )
    return remarks


@app.get("/health")
def health_check() -> dict[str, str]:
    return {"status": "ok"}


@app.post("/compile", response_model=CompileResponse)
def compile_code(request: CompileRequest) -> CompileResponse:
    compiler, suffix = _compiler_for(request.language)
    with tempfile.TemporaryDirectory() as temp_directory:
        temp_path = Path(temp_directory)
        source_file = temp_path / f"input{suffix}"
        output_file = temp_path / "output.ll"
        source_file.write_text(request.code, encoding="utf-8")

        started = time.perf_counter()
        result = _run([
            compiler,
            "-S",
            "-emit-llvm",
            f"-{request.optimization_level}",
            str(source_file),
            "-o",
            str(output_file),
        ])
        elapsed_ms = (time.perf_counter() - started) * 1000
        _raise_compile_error(result)

        return CompileResponse(
            success=True,
            llvm_ir=output_file.read_text(encoding="utf-8"),
            compiler_output=result.stderr,
            compile_time_ms=round(elapsed_ms, 3),
        )


@app.post("/profile", response_model=ProfileResponse)
def profile_code(request: CompileRequest) -> ProfileResponse:
    compiler, suffix = _compiler_for(request.language)
    opt = _find_opt() if request.optimization_level != "O0" else None

    with tempfile.TemporaryDirectory() as temp_directory:
        temp_path = Path(temp_directory)
        source_file = temp_path / f"input{suffix}"
        before_file = temp_path / "before.ll"
        after_file = temp_path / "after.ll"
        source_file.write_text(request.code, encoding="utf-8")

        compile_started = time.perf_counter()
        compile_result = _run([
            compiler,
            "-S",
            "-emit-llvm",
            "-O0",
            "-gline-tables-only",
            "-Xclang",
            "-disable-O0-optnone",
            str(source_file),
            "-o",
            str(before_file),
        ])
        compile_time_ms = (time.perf_counter() - compile_started) * 1000
        _raise_compile_error(compile_result)

        if request.optimization_level == "O0":
            after_file.write_text(before_file.read_text(encoding="utf-8"), encoding="utf-8")
            optimization_result = subprocess.CompletedProcess([], 0, "", "")
            optimization_time_ms = 0.0
        else:
            optimization_started = time.perf_counter()
            optimization_result = _run([
                opt or "opt",
                "-S",
                f"-passes=default<{request.optimization_level}>",
                "-time-passes",
                "-pass-remarks=.*",
                "-pass-remarks-missed=.*",
                "-pass-remarks-analysis=.*",
                str(before_file),
                "-o",
                str(after_file),
            ])
            optimization_time_ms = (time.perf_counter() - optimization_started) * 1000
            if optimization_result.returncode != 0:
                raise HTTPException(
                    status_code=500,
                    detail={"message": "LLVM optimization failed.", "compiler_output": optimization_result.stderr},
                )

        before_ir = before_file.read_text(encoding="utf-8")
        after_ir = after_file.read_text(encoding="utf-8")
        instruction_count_before, basic_block_count_before = _ir_metrics(before_ir)
        instruction_count_after, basic_block_count_after = _ir_metrics(after_ir)

        return ProfileResponse(
            compile_time_ms=round(compile_time_ms + optimization_time_ms, 3),
            optimization_time_ms=round(optimization_time_ms, 3),
            before_ir=before_ir,
            after_ir=after_ir,
            instruction_count_before=instruction_count_before,
            instruction_count_after=instruction_count_after,
            basic_block_count_before=basic_block_count_before,
            basic_block_count_after=basic_block_count_after,
            pass_timings=_parse_pass_timings(optimization_result.stderr),
            remarks=_parse_remarks(optimization_result.stderr),
        )

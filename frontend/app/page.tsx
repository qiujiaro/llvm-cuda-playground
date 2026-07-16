"use client";

import { useCallback, useEffect, useMemo, useState } from "react";

const DEFAULT_SOURCE = `#include <cstddef>

void add(
    const float* __restrict__ a,
    const float* __restrict__ b,
    float* __restrict__ c,
    std::size_t n
) {
    for (std::size_t i = 0; i < n; ++i) {
        c[i] = a[i] + b[i];
    }
}`;

const DEFAULT_IR = `; ModuleID = 'input.cpp'
source_filename = "input.cpp"
target triple = "x86_64-unknown-linux-gnu"

define dso_local void @_Z3addPKfS0_Pfm(
  ptr noalias nocapture readonly %a,
  ptr noalias nocapture readonly %b,
  ptr noalias nocapture writeonly %c,
  i64 %n
) local_unnamed_addr {
entry:
  %cmp7 = icmp eq i64 %n, 0
  br i1 %cmp7, label %for.cond.cleanup, label %vector.ph

vector.ph:
  %n.vec = and i64 %n, -8
  br label %vector.body

vector.body:
  %index = phi i64 [ 0, %vector.ph ], [ %index.next, %vector.body ]
  %0 = getelementptr inbounds float, ptr %a, i64 %index
  %wide.load = load <4 x float>, ptr %0, align 4
  %1 = getelementptr inbounds float, ptr %b, i64 %index
  %wide.load8 = load <4 x float>, ptr %1, align 4
  %2 = fadd <4 x float> %wide.load, %wide.load8
  store <4 x float> %2, ptr %c, align 4
  %index.next = add nuw i64 %index, 8
  %3 = icmp eq i64 %index.next, %n.vec
  br i1 %3, label %for.cond.cleanup, label %vector.body

for.cond.cleanup:
  ret void
}`;

const API_URL = process.env.NEXT_PUBLIC_API_URL ?? "http://localhost:8000";

type PassTiming = { name: string; time_ms: number };
type Remark = {
  kind: "passed" | "missed" | "analysis" | "warning";
  pass_name: string;
  message: string;
  file: string | null;
  line: number | null;
  column: number | null;
};
type ProfileResult = {
  compile_time_ms: number;
  optimization_time_ms: number;
  before_ir: string;
  after_ir: string;
  instruction_count_before: number;
  instruction_count_after: number;
  basic_block_count_before: number;
  basic_block_count_after: number;
  pass_timings: PassTiming[];
  remarks: Remark[];
};
type CompileResult = { success: boolean; llvm_ir: string; compiler_output: string; compile_time_ms: number };
type EngineResult = {
  success: boolean;
  llvm_ir: string;
  optimized_ir: string;
  ptx: string;
  assembly: string;
  diagnostics: string;
  function_count: number;
  basic_block_count: number;
  instruction_count: number;
  finding_count: number;
};

const INITIAL_PROFILE: ProfileResult = {
  compile_time_ms: 42,
  optimization_time_ms: 11,
  before_ir: DEFAULT_IR,
  after_ir: DEFAULT_IR,
  instruction_count_before: 68,
  instruction_count_after: 41,
  basic_block_count_before: 9,
  basic_block_count_after: 6,
  pass_timings: [
    { name: "SROAPass", time_ms: 0.9 },
    { name: "InstCombinePass", time_ms: 1.2 },
    { name: "SimplifyCFGPass", time_ms: 0.5 },
    { name: "LoopVectorizePass", time_ms: 3.1 },
    { name: "GVNPass", time_ms: 0.7 },
  ],
  remarks: [],
};

type AnalysisTab = "overview" | "passes" | "diff" | "remarks" | "profiling";
type OutputTab = "ir" | "compiler" | "engine";

async function readJson<T>(response: Response): Promise<T> {
  const data = await response.json();
  if (!response.ok) {
    const detail = data?.detail;
    throw new Error(detail?.compiler_output ?? detail?.message ?? detail ?? `Request failed (${response.status})`);
  }
  return data as T;
}

function Chevron() {
  return <svg viewBox="0 0 12 12" aria-hidden="true"><path d="m3 4.5 3 3 3-3" /></svg>;
}

function PlayIcon() {
  return <svg viewBox="0 0 16 16" aria-hidden="true"><path d="m5 3 7 5-7 5V3Z" /></svg>;
}

function CodePane({ value, onChange, readOnly = false }: { value: string; onChange?: (value: string) => void; readOnly?: boolean }) {
  const lines = value.split("\n");
  return (
    <div className="code-pane">
      <div className="line-numbers" aria-hidden="true">
        {lines.map((_, index) => <span key={index}>{index + 1}</span>)}
      </div>
      {readOnly ? (
        <pre className="code-content"><code>{value}</code></pre>
      ) : (
        <textarea
          aria-label="C++ source code"
          className="code-content code-input"
          value={value}
          onChange={(event) => onChange?.(event.target.value)}
          spellCheck={false}
        />
      )}
    </div>
  );
}

function Metric({ label, value, detail, tone }: { label: string; value: string; detail?: string; tone?: "good" | "blue" }) {
  return (
    <div className="metric">
      <span>{label}</span>
      <strong className={tone ? `tone-${tone}` : ""}>{value}</strong>
      {detail && <small>{detail}</small>}
    </div>
  );
}

export default function Home() {
  const [source, setSource] = useState(DEFAULT_SOURCE);
  const [output, setOutput] = useState(DEFAULT_IR);
  const [compilerOutput, setCompilerOutput] = useState("Compilation completed with 0 errors and 0 warnings.");
  const [engineOutput, setEngineOutput] = useState("Run the compiler to invoke the C++ engine.");
  const [outputTab, setOutputTab] = useState<OutputTab>("ir");
  const [analysisTab, setAnalysisTab] = useState<AnalysisTab>("overview");
  const [selectedPass, setSelectedPass] = useState(3);
  const [optimization, setOptimization] = useState("O2");
  const [target, setTarget] = useState("x86-64");
  const [running, setRunning] = useState(false);
  const [status, setStatus] = useState<"ready" | "success" | "error">("success");
  const [compileTime, setCompileTime] = useState(42);
  const [backendStatus, setBackendStatus] = useState<"checking" | "online" | "offline">("checking");
  const [profile, setProfile] = useState<ProfileResult>(INITIAL_PROFILE);

  const runCompile = useCallback(async () => {
    if (running) return;
    setRunning(true);
    setStatus("ready");
    try {
      const requestBody = JSON.stringify({ code: source, language: "cpp", optimization_level: optimization });
      const [compileResponse, profileResponse, engineResponse] = await Promise.all([
        fetch(`${API_URL}/compile`, {
          method: "POST",
          headers: { "Content-Type": "application/json" },
          body: requestBody,
        }),
        fetch(`${API_URL}/profile`, {
          method: "POST",
          headers: { "Content-Type": "application/json" },
          body: requestBody,
        }),
        fetch(`${API_URL}/engine/run`, {
          method: "POST",
          headers: { "Content-Type": "application/json" },
          body: requestBody,
        }),
      ]);
      const [compileData, profileData, engineData] = await Promise.all([
        readJson<CompileResult>(compileResponse),
        readJson<ProfileResult>(profileResponse),
        readJson<EngineResult>(engineResponse),
      ]);
      setOutput(compileData.llvm_ir);
      setCompilerOutput(compileData.compiler_output || "Compilation completed with 0 errors and 0 warnings.");
      setProfile(profileData);
      setEngineOutput([
        `Engine: ${engineData.success ? "success" : "failed"}`,
        `Functions: ${engineData.function_count}`,
        `Basic blocks: ${engineData.basic_block_count}`,
        `Instructions: ${engineData.instruction_count}`,
        `Findings: ${engineData.finding_count}`,
        engineData.diagnostics ? `\nDiagnostics:\n${engineData.diagnostics}` : "",
        `\nPTX:\n${engineData.ptx}`,
        `\nAssembly:\n${engineData.assembly}`,
      ].filter(Boolean).join("\n"));
      setOutputTab("ir");
      setStatus("success");
      setBackendStatus("online");
      setCompileTime(compileData.compile_time_ms);
    } catch (error) {
      const isNetworkError = error instanceof TypeError;
      setCompilerOutput(isNetworkError
        ? "Compiler service is offline. Start the backend on localhost:8000 and run again."
        : error instanceof Error ? error.message : "Compilation failed.");
      setOutputTab("compiler");
      setStatus("error");
      if (isNetworkError) setBackendStatus("offline");
    } finally {
      setRunning(false);
    }
  }, [optimization, running, source]);

  useEffect(() => {
    const controller = new AbortController();
    const checkHealth = async () => {
      try {
        const response = await fetch(`${API_URL}/health`, { signal: controller.signal });
        const data = await readJson<{ status: string }>(response);
        setBackendStatus(data.status === "ok" ? "online" : "offline");
      } catch {
        if (!controller.signal.aborted) setBackendStatus("offline");
      }
    };
    void checkHealth();
    return () => controller.abort();
  }, []);

  useEffect(() => {
    const onKeyDown = (event: KeyboardEvent) => {
      if ((event.metaKey || event.ctrlKey) && event.key === "Enter") {
        event.preventDefault();
        void runCompile();
      }
    };
    window.addEventListener("keydown", onKeyDown);
    return () => window.removeEventListener("keydown", onKeyDown);
  }, [runCompile]);

  const passes = profile.pass_timings;
  const selected = passes[Math.min(selectedPass, Math.max(0, passes.length - 1))];
  const lineCount = useMemo(() => source.split("\n").length, [source]);
  const instructionDelta = profile.instruction_count_after - profile.instruction_count_before;
  const instructionPercent = profile.instruction_count_before
    ? Math.abs(instructionDelta / profile.instruction_count_before * 100).toFixed(1)
    : "0.0";

  return (
    <main className="workbench">
      <header className="topbar">
        <div className="brand">
          <span className="brand-mark">IR</span>
          <span>IRScope</span>
          <span className="version-tag">BETA</span>
        </div>
        <nav className="main-nav" aria-label="Primary navigation">
          <a href="#workspace">Playground</a>
          <a href="#examples">Examples</a>
          <a href="#docs">Docs</a>
          <a href="#architecture">Architecture</a>
        </nav>
        <div className="top-actions">
          <span className={`engine-status ${backendStatus}`}>
            <i /> {backendStatus === "offline" ? "Backend unavailable" : backendStatus === "checking" ? "Checking backend…" : `LLVM ready · ${compileTime.toFixed(1)} ms`}
          </span>
          <button className="run-button" onClick={() => void runCompile()} disabled={running}>
            <PlayIcon />
            {running ? "Compiling…" : "Run"}
            <kbd>⌘ ↵</kbd>
          </button>
        </div>
      </header>

      <section className="editor-workspace" id="workspace">
        <div className="editor-panel source-panel">
          <div className="panel-heading">
            <div className="panel-title"><span>SOURCE</span><b>input.cpp</b></div>
            <div className="panel-tools">
              <button className="text-button">C++ <Chevron /></button>
              <button className="icon-button" aria-label="More source actions">•••</button>
            </div>
          </div>
          <CodePane value={source} onChange={setSource} />
          <div className="editor-statusbar">
            <span>C++17</span><span>{lineCount} lines</span>
            <span className={`compile-state ${status}`}><i />{status === "error" ? "Failed" : status === "ready" ? "Ready" : "Compiled"}</span>
            <span className="status-spacer" />
            <span>Ln 9, Col 5</span><span>Spaces: 4</span>
          </div>
        </div>

        <div className="editor-panel output-panel">
          <div className="panel-heading output-heading">
            <div className="output-tabs" role="tablist">
              <button className={outputTab === "ir" ? "active" : ""} onClick={() => setOutputTab("ir")}>LLVM IR</button>
              <button className={outputTab === "compiler" ? "active" : ""} onClick={() => setOutputTab("compiler")}>
                Compiler Output {status === "error" && <span className="error-dot" />}
              </button>
              <button className={outputTab === "engine" ? "active" : ""} onClick={() => setOutputTab("engine")}>Engine</button>
            </div>
            <button className="copy-button" onClick={() => void navigator.clipboard?.writeText(outputTab === "ir" ? output : outputTab === "compiler" ? compilerOutput : engineOutput)}>
              Copy
            </button>
          </div>
          {outputTab === "ir" ? (
            <CodePane value={output} readOnly />
          ) : (
            <div className={`compiler-console ${status === "error" ? "has-error" : ""}`}>
              <strong>{outputTab === "engine" ? "C++ Engine Output" : status === "error" ? "Compilation Failed" : "Compiler Output"}</strong>
              <pre>{outputTab === "engine" ? engineOutput : compilerOutput}</pre>
            </div>
          )}
          <div className="editor-statusbar output-status">
            <span>LLVM IR</span><span>{output.split("\n").length} lines</span>
            <span className="status-spacer" /><span>UTF-8</span><span>LF</span>
          </div>
        </div>
      </section>

      <section className="compiler-toolbar" aria-label="Compiler settings">
        <div className="setting"><label>LANGUAGE</label><button>C++17 <Chevron /></button></div>
        <div className="setting"><label>OPTIMIZATION</label><select value={optimization} onChange={(e) => setOptimization(e.target.value)}><option>O0</option><option>O1</option><option>O2</option><option>O3</option><option>Os</option></select></div>
        <div className="setting"><label>TARGET</label><select value={target} onChange={(e) => setTarget(e.target.value)}><option>x86-64</option><option>ARM64</option><option>RISC-V 64</option></select></div>
        <div className="setting disabled"><label>COMPILER</label><button disabled>Clang 18 <Chevron /></button></div>
        <div className="setting disabled wide-setting"><label>PIPELINE</label><button disabled>default&lt;{optimization.toLowerCase()}&gt; <Chevron /></button></div>
        <div className="toolbar-spacer" />
        <label className="check-control"><input type="checkbox" defaultChecked /><span />Optimization remarks</label>
      </section>

      <section className="analysis">
        <div className="analysis-tabs" role="tablist">
          {(["overview", "passes", "diff", "remarks", "profiling"] as const).map((tab) => (
            <button key={tab} className={analysisTab === tab ? "active" : ""} onClick={() => setAnalysisTab(tab)}>
              {tab === "overview" ? "Overview" : tab === "passes" ? "Pass Timeline" : tab === "diff" ? "IR Diff" : tab === "remarks" ? "Optimization Remarks" : "Pass Profiling"}
            </button>
          ))}
          <div className="analysis-run-meta"><i className={status === "error" ? "bad" : ""} /> Last run · just now</div>
        </div>

        <div className="analysis-body">
          {analysisTab === "overview" && (
            <div className="overview-grid">
              <div className="summary-section">
                <div className="section-label"><span>COMPILATION SUMMARY</span><small>{target} · {optimization}</small></div>
                <div className="metrics-row">
                  <Metric label="Compile time" value={`${compileTime.toFixed(1)} ms`} />
                  <Metric label="Optimization" value={`${profile.optimization_time_ms.toFixed(1)} ms`} />
                  <Metric label="IR instructions" value={`${profile.instruction_count_before} → ${profile.instruction_count_after}`} detail={`${instructionDelta <= 0 ? "−" : "+"}${instructionPercent}%`} tone={instructionDelta <= 0 ? "good" : "blue"} />
                  <Metric label="Basic blocks" value={`${profile.basic_block_count_before} → ${profile.basic_block_count_after}`} detail={`${profile.basic_block_count_after - profile.basic_block_count_before} blocks`} tone="good" />
                  <Metric label="Passes measured" value={`${passes.length}`} />
                  <Metric label="Remarks" value={`${profile.remarks.length}`} />
                </div>
              </div>
              <div className="changes-section">
                <div className="section-label"><span>OPTIMIZATION DECISIONS</span><small>{profile.remarks.length} remarks</small></div>
                <ul className="change-list">
                  {profile.remarks.slice(0, 4).map((remark, index) => (
                    <li key={`${remark.pass_name}-${remark.line}-${index}`}>
                      <span className={`change-icon ${remark.kind === "passed" ? "good" : "warn"}`}>{remark.kind === "passed" ? "✓" : "!"}</span>
                      <div><strong>{remark.message}</strong><small>{remark.pass_name}</small></div>
                      {remark.line && <b>line {remark.line}</b>}
                    </li>
                  ))}
                  {profile.remarks.length === 0 && <li className="empty-row">No optimization remarks were emitted for this source.</li>}
                </ul>
              </div>
              <div className="mini-timeline">
                <div className="section-label"><span>PASS EXECUTION</span><button onClick={() => setAnalysisTab("passes")}>View timeline →</button></div>
                <div className="bars">
                  {passes.slice(0, 8).map((pass, index) => <button key={`${pass.name}-${index}`} onClick={() => { setSelectedPass(index); setAnalysisTab("passes"); }}><span style={{ height: `${Math.max(12, Math.min(62, pass.time_ms * 12))}px` }} /><small>{pass.name.replace(/Pass$/, "")}</small><b>{pass.time_ms.toFixed(2)} ms</b></button>)}
                </div>
              </div>
            </div>
          )}

          {analysisTab === "passes" && (
            <div className="passes-view">
              <div className="pass-track">
                {passes.slice(0, 8).map((pass, index) => (
                  <button key={`${pass.name}-${index}`} className={selectedPass === index ? "active" : ""} onClick={() => setSelectedPass(index)}>
                    <span>{index + 1}</span><strong>{pass.name.replace(/Pass$/, "")}</strong><small>{pass.time_ms.toFixed(2)} ms</small>
                  </button>
                ))}
              </div>
              {selected ? <div className="pass-detail">
                <div><span>SELECTED PASS</span><h2>{selected.name}</h2><p>Measured by LLVM&apos;s pass timing instrumentation during the selected optimization pipeline.</p></div>
                <Metric label="Duration" value={`${selected.time_ms.toFixed(3)} ms`} tone="blue" />
                <Metric label="Pipeline" value={optimization} detail="default pipeline" />
                <Metric label="Target" value={target} />
              </div> : <div className="analysis-empty">No pass timing data was emitted.</div>}
            </div>
          )}

          {analysisTab === "diff" && (
            <div className="diff-view">
              <div className="diff-toolbar"><span>COMPARE</span><button>Initial IR <Chevron /></button><b>→</b><button>After {optimization} <Chevron /></button><span className="diff-stats"><i className="added" /> {profile.instruction_count_after} after <i className="removed" /> {profile.instruction_count_before} before</span></div>
              <div className="diff-columns">
                <div><h3>BEFORE OPTIMIZATION <span>{profile.instruction_count_before} instructions</span></h3><pre>{profile.before_ir}</pre></div>
                <div><h3>AFTER {optimization} <span>{profile.instruction_count_after} instructions</span></h3><pre>{profile.after_ir}</pre></div>
              </div>
            </div>
          )}

          {analysisTab === "remarks" && (
            <div className="remarks-view">
              <div className="section-label"><span>OPTIMIZATION REMARKS</span><small>{profile.remarks.length} compiler decisions</small></div>
              <div className="remarks-list">
                {profile.remarks.map((remark, index) => (
                  <article key={`${remark.pass_name}-${remark.line}-${index}`} className={`remark ${remark.kind}`}>
                    <span>{remark.kind.toUpperCase()}</span>
                    <div><strong>{remark.pass_name}</strong><p>{remark.message}</p></div>
                    <code>{remark.file ?? "input.cpp"}{remark.line ? `:${remark.line}:${remark.column ?? 1}` : ""}</code>
                  </article>
                ))}
                {profile.remarks.length === 0 && <div className="analysis-empty">No optimization remarks were emitted. Try a loop-oriented example at O2 or O3.</div>}
              </div>
            </div>
          )}

          {analysisTab === "profiling" && (
            <div className="profiling-view">
              <div className="section-label"><span>PASS EXECUTION TIME</span><small>Total request optimization time {profile.optimization_time_ms.toFixed(2)} ms</small></div>
              <table><thead><tr><th>Pass</th><th>Time</th><th>Relative time</th></tr></thead>
                <tbody>{passes.map((pass, index) => <tr key={`${pass.name}-${index}`}><td><i style={{ opacity: Math.max(.25, 1 - index * .04) }} />{pass.name}</td><td>{pass.time_ms.toFixed(3)} ms</td><td><span className="share-bar"><b style={{ width: `${Math.min(100, pass.time_ms / Math.max(...passes.map(item => item.time_ms), 0.001) * 100)}%` }} /></span></td></tr>)}</tbody>
              </table>
              <p className="profiling-note"><span>i</span> An increase in IR instructions is not always a regression. Vectorization can expand IR while improving runtime performance.</p>
            </div>
          )}
        </div>
      </section>
    </main>
  );
}

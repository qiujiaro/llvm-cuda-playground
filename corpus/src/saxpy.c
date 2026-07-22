// SAXPY: memory-bound streaming kernel. y = a*x + y.
// Compiled to LLVM IR with -O1 (clean SSA) and fed to the ExtractDAG pass.
void saxpy(float a, const float* x, float* y, int n) {
    for (int i = 0; i < n; ++i) {
        y[i] = a * x[i] + y[i];
    }
}

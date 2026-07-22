// 1D 3-point stencil: several independent loads -> high ILP, good scheduling
// opportunity for latency hiding.
void stencil(const float* in, float* out, int n) {
    for (int i = 1; i < n - 1; ++i) {
        out[i] = 0.25f * in[i - 1] + 0.5f * in[i] + 0.25f * in[i + 1];
    }
}

// Dot product: reduction with a serial dependency chain on the accumulator.
float dotprod(const float* a, const float* b, int n) {
    float acc = 0.0f;
    for (int i = 0; i < n; ++i) {
        acc += a[i] * b[i];
    }
    return acc;
}

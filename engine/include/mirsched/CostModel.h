#pragma once

// SIMT-flavoured latency / occupancy cost model.
//
// This model is DIRECTIONAL, not a hardware simulator. See docs/COST_MODEL.md
// for the assumptions and their limits. Do NOT claim these numbers equal real
// GPU performance.

#include <string>

namespace mirsched {

struct Node;

class CostModel {
public:
    // Configurable knobs (defaults are placeholders; tune per experiment).
    int aluLatency = 1;      // add/sub/mul/logic ...
    int memLatency = 8;      // load/store (raise this to model global memory)
    int specialLatency = 4;  // div / sqrt / transcendental-ish
    int issueWidth = 2;      // instructions issuable per cycle
    int regFileSize = 65536; // registers per SM-equivalent (occupancy model)
    int threadsPerWarp = 32;

    // Latency of a single node under this model.
    int latencyOf(const Node& node) const;

    // Directional occupancy estimate from peak register pressure. Documented as
    // an approximation in docs/COST_MODEL.md -- not a real occupancy number.
    double estimateOccupancy(int peakPressure) const;

    static CostModel fromJsonFile(const std::string& path);
};

}  // namespace mirsched

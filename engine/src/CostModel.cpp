#include "mirsched/CostModel.h"

#include "mirsched/Dag.h"
#include "mirsched/Json.h"

#include <algorithm>

namespace mirsched {

int CostModel::latencyOf(const Node& node) const {
    if (node.isLoad || node.isStore) return memLatency;

    const std::string& op = node.opcode;
    if (op == "fdiv" || op == "sdiv" || op == "udiv" || op == "call"
        || op == "sqrt") {
        return specialLatency;
    }
    return aluLatency;
}

double CostModel::estimateOccupancy(int peakPressure) const {
    // Directional model: more registers live simultaneously => fewer concurrent
    // warps can be resident. NOT a real occupancy figure -- see docs/COST_MODEL.md.
    if (peakPressure <= 0) return 1.0;
    const double registersPerWarp =
        static_cast<double>(peakPressure) * threadsPerWarp;
    if (registersPerWarp <= 0) return 1.0;
    const double warps = static_cast<double>(regFileSize) / registersPerWarp;
    // Normalize against an arbitrary "max warps" reference of 64 for a 0..1 view.
    return std::min(1.0, warps / 64.0);
}

CostModel CostModel::fromJsonFile(const std::string& path) {
    CostModel cm;
    const JsonValue root = parseJsonFile(path);
    if (!root.isObject()) return cm;
    cm.aluLatency = static_cast<int>(root["aluLatency"].asInt(cm.aluLatency));
    cm.memLatency = static_cast<int>(root["memLatency"].asInt(cm.memLatency));
    cm.specialLatency =
        static_cast<int>(root["specialLatency"].asInt(cm.specialLatency));
    cm.issueWidth = static_cast<int>(root["issueWidth"].asInt(cm.issueWidth));
    cm.regFileSize = static_cast<int>(root["regFileSize"].asInt(cm.regFileSize));
    cm.threadsPerWarp =
        static_cast<int>(root["threadsPerWarp"].asInt(cm.threadsPerWarp));
    return cm;
}

}  // namespace mirsched

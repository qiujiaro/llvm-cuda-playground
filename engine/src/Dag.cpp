#include "mirsched/Dag.h"

#include "mirsched/CostModel.h"
#include "mirsched/Json.h"

#include <algorithm>
#include <stdexcept>

namespace mirsched {

namespace {

DepKind kindFromString(const std::string& s) {
    if (s == "memory") return DepKind::Memory;
    if (s == "anti") return DepKind::Anti;
    if (s == "output") return DepKind::Output;
    return DepKind::Data;
}

}  // namespace

Dag Dag::fromJsonFile(const std::string& path) {
    const JsonValue root = parseJsonFile(path);
    if (!root.isObject()) {
        throw std::runtime_error("DAG file is not a JSON object: " + path);
    }

    Dag dag;
    dag.function = root["function"].asString();
    dag.block = root["block"].asString();

    const JsonValue& nodes = root["nodes"];
    if (nodes.isArray()) {
        for (const JsonValue& n : nodes.arrayValue) {
            Node node;
            node.id = static_cast<int>(n["id"].asInt(-1));
            node.opcode = n["opcode"].asString("unknown");
            node.isLoad = n["isLoad"].asBool(false);
            node.isStore = n["isStore"].asBool(false);
            dag.nodes.push_back(node);
        }
    }

    const JsonValue& edges = root["edges"];
    if (edges.isArray()) {
        for (const JsonValue& e : edges.arrayValue) {
            Edge edge;
            edge.from = static_cast<int>(e["from"].asInt(-1));
            edge.to = static_cast<int>(e["to"].asInt(-1));
            edge.kind = kindFromString(e["kind"].asString("data"));
            dag.edges.push_back(edge);
        }
    }

    dag.normalize();
    dag.buildAdjacency();
    return dag;
}

void Dag::normalize() {
    std::sort(nodes.begin(), nodes.end(),
              [](const Node& a, const Node& b) { return a.id < b.id; });
    // Assume ids are already 0..n-1 as emitted by ExtractDAG; validate loosely.
    for (std::size_t i = 0; i < nodes.size(); ++i) {
        if (nodes[i].id < 0) nodes[i].id = static_cast<int>(i);
    }
}

void Dag::buildAdjacency() {
    const std::size_t n = nodes.size();
    succ.assign(n, {});
    pred.assign(n, {});
    for (const Edge& e : edges) {
        if (e.from < 0 || e.to < 0 || e.from >= static_cast<int>(n)
            || e.to >= static_cast<int>(n)) {
            continue;
        }
        succ[e.from].push_back(e.to);
        pred[e.to].push_back(e.from);
    }
}

void Dag::applyLatencies(const CostModel& cm) {
    for (Node& node : nodes) {
        node.latency = cm.latencyOf(node);
    }
}

void Dag::addMemoryEdges() {
    // Conservative program-order memory dependencies: for every ordered pair
    // (a, b) with a.id < b.id, add a memory edge when at least one is a store
    // (store->store, store->load, load->store). load->load is independent.
    std::vector<int> memNodes;
    for (const Node& node : nodes) {
        if (node.isLoad || node.isStore) memNodes.push_back(node.id);
    }
    for (std::size_t i = 0; i < memNodes.size(); ++i) {
        for (std::size_t j = i + 1; j < memNodes.size(); ++j) {
            const Node& a = nodes[memNodes[i]];
            const Node& b = nodes[memNodes[j]];
            const bool independent = a.isLoad && b.isLoad;
            if (independent) continue;
            edges.push_back(Edge{a.id, b.id, DepKind::Memory});
        }
    }
    buildAdjacency();
}

bool Dag::computeCriticalPath() {
    // ------------------------------------------------------------------
    // Day-3 TASK: implement critical-path height via reverse-topological DP.
    //
    //   for n in reverse-topological order:
    //       cpHeight[n] = latency[n] + max(0, max over s in succ[n] cpHeight[s])
    //
    // Detect cycles (Kahn's algorithm running out of zero-in-degree nodes, or a
    // DFS "gray" node revisit) and return false if the graph is not a DAG.
    //
    // Store the result into nodes[i].cpHeight. Add unit tests in
    // tests/test_scheduler.cpp against tests/golden/*.json.
    //
    // The stub below marks every node as "not computed" so downstream code can
    // detect that this task is still pending.
    // ------------------------------------------------------------------
    for (Node& node : nodes) node.cpHeight = -1;
    return false;  // TODO: replace with your real implementation
}

}  // namespace mirsched

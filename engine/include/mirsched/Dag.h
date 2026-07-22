#pragma once

// Scheduling DAG (Module 1 in the 10-Day guide).
//
// The DATA STRUCTURE and I/O here are provided scaffolding.
// The CRITICAL-PATH computation (computeCriticalPath) is a Day-3 task you must
// implement yourself -- it is one of the guide's "hand-implemented" core pieces.

#include <string>
#include <vector>

namespace mirsched {

class CostModel;  // forward decl

enum class DepKind { Data, Memory, Anti, Output };

struct Node {
    int id = -1;
    std::string opcode;
    bool isLoad = false;
    bool isStore = false;
    int latency = 1;       // filled from CostModel before scheduling
    long cpHeight = -1;    // critical-path height; -1 == not computed yet
};

struct Edge {
    int from = -1;
    int to = -1;
    DepKind kind = DepKind::Data;
};

class Dag {
public:
    std::string function;
    std::string block;
    std::vector<Node> nodes;   // node[i].id == i is assumed after normalize()
    std::vector<Edge> edges;

    // Adjacency lists indexed by node id (built by buildAdjacency()).
    std::vector<std::vector<int>> succ;
    std::vector<std::vector<int>> pred;

    // Load from the JSON DAG format emitted by the ExtractDAG LLVM pass.
    static Dag fromJsonFile(const std::string& path);

    // Ensure ids are contiguous 0..n-1 and build succ/pred adjacency.
    void normalize();
    void buildAdjacency();

    // Apply per-opcode latencies from the cost model onto every node.
    void applyLatencies(const CostModel& cm);

    // Add conservative memory-ordering edges among load/store nodes in program
    // (id) order. Provided as scaffolding; you may harden it on Day 3.
    void addMemoryEdges();

    std::size_t size() const { return nodes.size(); }

    // ------------------------------------------------------------------
    // Day-3 TASK (implement yourself): critical-path height.
    //   cpHeight(n) = latency(n) + max over successors s of cpHeight(s)
    //   (a leaf's cpHeight == its own latency)
    // Compute it via reverse-topological dynamic programming and store it in
    // node.cpHeight. Return false if the graph is not a DAG (cycle detected).
    // Requires applyLatencies() + buildAdjacency() to have run first.
    // ------------------------------------------------------------------
    bool computeCriticalPath();
};

}  // namespace mirsched

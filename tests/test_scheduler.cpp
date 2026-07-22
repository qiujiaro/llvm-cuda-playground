#include "test_framework.h"

#include "mirsched/CostModel.h"
#include "mirsched/Dag.h"
#include "mirsched/ListScheduler.h"

#include <string>

using namespace mirsched;

namespace {

std::string golden(const std::string& name) {
    return std::string(MIRSCHED_GOLDEN_DIR) + "/" + name;
}

Dag loadPrepared(const std::string& name, const CostModel& cm) {
    Dag dag = Dag::fromJsonFile(golden(name));
    dag.addMemoryEdges();
    dag.applyLatencies(cm);
    return dag;
}

// Verify every dependency edge is honored: issue(u) + latency(u) <= issue(v).
bool respectsDeps(const Dag& dag, const ScheduleResult& s) {
    for (const Edge& e : dag.edges) {
        if (s.issueCycle[e.from] + dag.nodes[e.from].latency
            > s.issueCycle[e.to]) {
            return false;
        }
    }
    return true;
}

}  // namespace

TEST(json_and_dag_parse) {
    Dag dag = Dag::fromJsonFile(golden("chain.json"));
    CHECK_EQ(dag.function, std::string("chain"));
    CHECK_EQ(dag.size(), static_cast<std::size_t>(4));
    CHECK(dag.nodes[0].isLoad);
    CHECK(dag.nodes[3].isStore);
    CHECK_EQ(dag.edges.size(), static_cast<std::size_t>(3));
}

TEST(cost_model_latencies) {
    CostModel cm;  // defaults: alu=1, mem=8
    Dag dag = Dag::fromJsonFile(golden("chain.json"));
    dag.applyLatencies(cm);
    CHECK_EQ(dag.nodes[0].latency, cm.memLatency);  // load
    CHECK_EQ(dag.nodes[1].latency, cm.aluLatency);  // fmul
    CHECK_EQ(dag.nodes[3].latency, cm.memLatency);  // store
}

TEST(program_order_chain_length) {
    CostModel cm;
    Dag dag = loadPrepared("chain.json", cm);
    ScheduleResult s = ListScheduler(dag, cm).run(SchedPolicy::ProgramOrder);
    CHECK(s.valid);
    CHECK(respectsDeps(dag, s));
    // load(8) -> fmul@8 -> fadd@9 -> store@10, +8 latency = 18 cycles.
    CHECK_EQ(s.length, 18L);
}

TEST(program_order_diamond_length) {
    CostModel cm;
    Dag dag = loadPrepared("diamond.json", cm);
    ScheduleResult s = ListScheduler(dag, cm).run(SchedPolicy::ProgramOrder);
    CHECK(s.valid);
    CHECK(respectsDeps(dag, s));
    // load(8) -> two fmul issue @8 (width 2) -> fadd @9, +1 = 10 cycles.
    CHECK_EQ(s.length, 10L);
}

// Activated once you implement Day-4 list scheduling: it must never be worse
// than the program-order baseline on any DAG.
TEST(list_no_worse_than_baseline) {
    CostModel cm;
    Dag dag = loadPrepared("chain.json", cm);
    ListScheduler sch(dag, cm);
    ScheduleResult base = sch.run(SchedPolicy::ProgramOrder);
    ScheduleResult list = sch.run(SchedPolicy::ListCriticalPath);
    if (!list.valid) SKIP("ListCriticalPath not implemented yet (Day 4)");
    CHECK(respectsDeps(dag, list));
    CHECK(list.length <= base.length);
}

// Activated once you implement Day-3 critical path.
TEST(critical_path_chain) {
    CostModel cm;
    Dag dag = loadPrepared("chain.json", cm);
    if (!dag.computeCriticalPath()) SKIP("computeCriticalPath not implemented (Day 3)");
    // Chain: store(8) then fadd(1) then fmul(1) then load(8) accumulate upward.
    // Leaf store cpHeight = 8; root load cpHeight = 8+1+1+8 = 18.
    CHECK_EQ(dag.nodes[0].cpHeight, 18L);
    CHECK_EQ(dag.nodes[3].cpHeight, 8L);
}

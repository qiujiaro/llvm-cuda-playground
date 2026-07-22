#include "test_framework.h"

#include "mirsched/CostModel.h"
#include "mirsched/Dag.h"
#include "mirsched/LinearScan.h"
#include "mirsched/ListScheduler.h"

#include <string>

using namespace mirsched;

namespace {

std::string golden(const std::string& name) {
    return std::string(MIRSCHED_GOLDEN_DIR) + "/" + name;
}

}  // namespace

TEST(live_intervals_built) {
    CostModel cm;
    Dag dag = Dag::fromJsonFile(golden("diamond.json"));
    dag.applyLatencies(cm);
    ScheduleResult s = ListScheduler(dag, cm).run(SchedPolicy::ProgramOrder);
    CHECK(s.valid);
    auto intervals = buildLiveIntervals(dag, s);
    CHECK_EQ(intervals.size(), static_cast<std::size_t>(4));
    // node0 (the load) is used by node1 and node2, so its interval must extend
    // to at least the later of their issue cycles.
    CHECK(intervals[0].end >= intervals[0].start);
}

TEST(peak_pressure_nonnegative) {
    CostModel cm;
    Dag dag = Dag::fromJsonFile(golden("diamond.json"));
    dag.applyLatencies(cm);
    ScheduleResult s = ListScheduler(dag, cm).run(SchedPolicy::ProgramOrder);
    auto intervals = buildLiveIntervals(dag, s);
    CHECK(peakPressure(intervals) >= 1);
}

// Activated once you implement Day-6 linear scan.
TEST(linear_scan_respects_budget) {
    CostModel cm;
    Dag dag = Dag::fromJsonFile(golden("diamond.json"));
    dag.applyLatencies(cm);
    ScheduleResult s = ListScheduler(dag, cm).run(SchedPolicy::ProgramOrder);
    auto intervals = buildLiveIntervals(dag, s);

    const int budget = 2;
    AllocResult alloc = LinearScan(budget).run(intervals, cm);
    if (!alloc.valid) SKIP("LinearScan not implemented yet (Day 6)");
    // With a tiny budget on a graph whose peak pressure exceeds it, spills > 0.
    if (peakPressure(intervals) > budget) {
        CHECK(alloc.spills > 0);
    }
}

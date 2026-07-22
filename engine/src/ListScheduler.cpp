#include "mirsched/ListScheduler.h"

#include "mirsched/CostModel.h"
#include "mirsched/Dag.h"

#include <algorithm>

namespace mirsched {

ScheduleResult ListScheduler::run(SchedPolicy policy) const {
    switch (policy) {
        case SchedPolicy::ProgramOrder: return runProgramOrder();
        case SchedPolicy::ListCriticalPath: return runListCriticalPath();
        case SchedPolicy::ListPressureAware: return runListPressureAware();
    }
    return {};
}

// -------------------------------------------------------------------------
// BASELINE (provided): in-order issue that still respects dependency latencies.
// A node cannot issue before every predecessor's result is ready, and at most
// issueWidth nodes issue per cycle. This is a legitimate, honest baseline --
// it measures the cost of NOT reordering.
// -------------------------------------------------------------------------
ScheduleResult ListScheduler::runProgramOrder() const {
    const std::size_t n = dag_.size();
    ScheduleResult result;
    result.issueCycle.assign(n, 0);
    if (n == 0) {
        result.valid = true;
        return result;
    }

    long cursor = 0;              // earliest cycle the next in-order node may use
    int issuedThisCycle = 0;
    const int width = std::max(1, cm_.issueWidth);

    for (std::size_t id = 0; id < n; ++id) {
        // Earliest cycle allowed by data/memory dependencies.
        long ready = cursor;
        for (int p : dag_.pred[id]) {
            ready = std::max(ready,
                             static_cast<long>(result.issueCycle[p])
                                 + dag_.nodes[p].latency);
        }
        // Respect issue width within a cycle (in-order).
        if (ready == cursor && issuedThisCycle >= width) {
            cursor += 1;
            ready = cursor;
            issuedThisCycle = 0;
        }
        if (ready > cursor) {
            cursor = ready;
            issuedThisCycle = 0;
        }
        result.issueCycle[id] = static_cast<int>(cursor);
        ++issuedThisCycle;
    }

    long length = 0;
    for (std::size_t id = 0; id < n; ++id) {
        length = std::max(length,
                          static_cast<long>(result.issueCycle[id])
                              + dag_.nodes[id].latency);
    }
    result.length = length;
    result.valid = true;
    return result;
}

// -------------------------------------------------------------------------
// Day-4 TASK (implement yourself): critical-path list scheduling.
//
//   compute cpHeight for all nodes (Dag::computeCriticalPath)
//   ready = { n : indegree(n) == 0 }
//   cycle = 0
//   while not all scheduled:
//       avail = { n in ready : readyCycle[n] <= cycle }
//       sort avail by cpHeight desc (tie-break later, Day 5)
//       issue up to issueWidth nodes
//       for each issued n, for s in succ(n):
//           readyCycle[s] = max(readyCycle[s], cycle + latency(n))
//           if all preds of s scheduled: ready.add(s)
//       cycle++
//
// Invariant to assert in tests: for every edge (u->v),
//   issueCycle[u] + latency[u] <= issueCycle[v].
// And length(list) <= length(programOrder) on every DAG.
// -------------------------------------------------------------------------
ScheduleResult ListScheduler::runListCriticalPath() const {
    ScheduleResult result;
    result.valid = false;  // TODO: replace with your real implementation
    return result;
}

// -------------------------------------------------------------------------
// Day-5 TASK (implement yourself): add a register-pressure-aware tie-break.
// Same loop as above, but when cpHeight ties, prefer the node whose issue least
// increases the current live set. Expect peak pressure / spills to drop, at a
// possible small cost in schedule length (the classic trade-off, Experiment 2).
// -------------------------------------------------------------------------
ScheduleResult ListScheduler::runListPressureAware() const {
    ScheduleResult result;
    result.valid = false;  // TODO: replace with your real implementation
    return result;
}

}  // namespace mirsched

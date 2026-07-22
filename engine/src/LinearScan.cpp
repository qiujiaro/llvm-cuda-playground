#include "mirsched/LinearScan.h"

#include "mirsched/CostModel.h"
#include "mirsched/Dag.h"
#include "mirsched/ListScheduler.h"

#include <algorithm>

namespace mirsched {

// -------------------------------------------------------------------------
// PROVIDED: live-interval construction.
// Each node defines one virtual register (its result). The interval starts at
// the node's issue cycle and ends at the latest issue cycle among its users
// (successors in the data DAG). Values with no users get a zero-length interval.
// -------------------------------------------------------------------------
std::vector<LiveInterval> buildLiveIntervals(const Dag& dag,
                                             const ScheduleResult& sched) {
    std::vector<LiveInterval> intervals;
    if (!sched.valid) return intervals;

    const std::size_t n = dag.size();
    for (std::size_t id = 0; id < n; ++id) {
        LiveInterval iv;
        iv.vreg = static_cast<int>(id);
        iv.start = sched.issueCycle[id];
        iv.end = sched.issueCycle[id];
        for (int s : dag.succ[id]) {
            iv.end = std::max(iv.end, sched.issueCycle[s]);
        }
        intervals.push_back(iv);
    }
    return intervals;
}

// -------------------------------------------------------------------------
// PROVIDED: peak register pressure via a sweep line over [start, end] intervals.
// -------------------------------------------------------------------------
int peakPressure(const std::vector<LiveInterval>& intervals) {
    if (intervals.empty()) return 0;
    struct Event {
        int cycle;
        int delta;
    };
    std::vector<Event> events;
    events.reserve(intervals.size() * 2);
    for (const LiveInterval& iv : intervals) {
        events.push_back({iv.start, +1});
        events.push_back({iv.end + 1, -1});  // end is inclusive
    }
    std::sort(events.begin(), events.end(), [](const Event& a, const Event& b) {
        if (a.cycle != b.cycle) return a.cycle < b.cycle;
        return a.delta < b.delta;  // process expirations before opens at same cycle
    });
    int live = 0;
    int peak = 0;
    for (const Event& e : events) {
        live += e.delta;
        peak = std::max(peak, live);
    }
    return peak;
}

// -------------------------------------------------------------------------
// Day-6 TASK (implement yourself): linear-scan register allocation + spills.
// See the header for the algorithm outline. The stub below reports peak pressure
// (from the provided helper) and the occupancy estimate, but leaves spill
// allocation unimplemented so downstream code can detect that it's pending.
// -------------------------------------------------------------------------
AllocResult LinearScan::run(const std::vector<LiveInterval>& intervals,
                            const CostModel& cm) const {
    AllocResult result;
    result.peakPressure = peakPressure(intervals);
    result.estOccupancy = cm.estimateOccupancy(result.peakPressure);
    result.spills = 0;      // TODO: compute via linear scan
    result.valid = false;   // TODO: set true once you implement the scan loop
    return result;
}

}  // namespace mirsched

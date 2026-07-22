#pragma once

// Register allocator (Module 3 in the 10-Day guide).
//
// Live-interval construction and peak-pressure sweep are provided scaffolding.
// The linear-scan allocation loop + spill selection is a Day-6 task you must
// implement yourself.

#include <vector>

namespace mirsched {

class Dag;
class CostModel;
struct ScheduleResult;

struct LiveInterval {
    int vreg = -1;   // the SSA value / node id that defines this register
    int start = 0;   // cycle of definition
    int end = 0;     // cycle of last use (== start if never used)
};

struct AllocResult {
    bool valid = false;
    int spills = 0;
    int peakPressure = 0;
    double estOccupancy = 0.0;
};

// Provided: derive live intervals from a schedule + the DAG's def/use structure.
std::vector<LiveInterval> buildLiveIntervals(const Dag& dag,
                                             const ScheduleResult& sched);

// Provided: maximum number of intervals simultaneously live at any cycle.
int peakPressure(const std::vector<LiveInterval>& intervals);

class LinearScan {
public:
    explicit LinearScan(int numRegs) : numRegs_(numRegs) {}

    // Day-6 TASK (implement yourself): classic linear-scan allocation.
    //   sort intervals by start
    //   active = [] (sorted by end); free = {p0..p_{K-1}}
    //   for iv in intervals:
    //       expire intervals whose end < iv.start (return their regs)
    //       if |active| == K: spill the active interval with the farthest end
    //                         (spill iv itself if its end is farther)
    //       else: assign a free reg
    //   count spills; peakPressure via the helper above; occupancy via CostModel
    // Invariant to test: at any cycle, #(live, non-spilled) <= numRegs.
    AllocResult run(const std::vector<LiveInterval>& intervals,
                    const CostModel& cm) const;

private:
    int numRegs_ = 0;
};

}  // namespace mirsched

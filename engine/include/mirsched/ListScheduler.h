#pragma once

// Instruction scheduler (Module 2 in the 10-Day guide).
//
// ProgramOrder (baseline) is fully implemented as reference scaffolding.
// ListCriticalPath and ListPressureAware are Day-4 / Day-5 tasks you must
// implement yourself -- they are the heart of the project and the main
// interview surface.

#include <vector>

namespace mirsched {

class Dag;
class CostModel;

enum class SchedPolicy {
    ProgramOrder,       // baseline: in-order issue, respects latencies
    ListCriticalPath,   // TODO (Day 4): critical-path list scheduling
    ListPressureAware,  // TODO (Day 5): + register-pressure tie-break
};

struct ScheduleResult {
    bool valid = false;
    long length = 0;                 // total cycles under the cost model
    std::vector<int> issueCycle;     // issueCycle[nodeId] = cycle it issues
    long stallCycles = 0;            // cycles with no issue (optional metric)
};

class ListScheduler {
public:
    ListScheduler(const Dag& dag, const CostModel& cm) : dag_(dag), cm_(cm) {}

    ScheduleResult run(SchedPolicy policy) const;

private:
    const Dag& dag_;
    const CostModel& cm_;

    ScheduleResult runProgramOrder() const;      // provided
    ScheduleResult runListCriticalPath() const;   // TODO (you)
    ScheduleResult runListPressureAware() const;  // TODO (you)
};

}  // namespace mirsched

// mirsched CLI: read a DAG, schedule it, allocate registers, print metrics.
//
// Usage:
//   mirsched <dag.json> [--sched=program|list|list-pressure]
//                       [--regs=K] [--config=cost.json] [--csv]
//
// With --csv it prints a single results row:
//   function,policy,regs,sched_length,peak_pressure,spills,est_occupancy,valid

#include "mirsched/CostModel.h"
#include "mirsched/Dag.h"
#include "mirsched/LinearScan.h"
#include "mirsched/ListScheduler.h"

#include <iostream>
#include <string>

namespace {

std::string argValue(const std::string& arg, const std::string& key) {
    const std::string prefix = key + "=";
    if (arg.rfind(prefix, 0) == 0) return arg.substr(prefix.size());
    return "";
}

mirsched::SchedPolicy parsePolicy(const std::string& s) {
    if (s == "list") return mirsched::SchedPolicy::ListCriticalPath;
    if (s == "list-pressure") return mirsched::SchedPolicy::ListPressureAware;
    return mirsched::SchedPolicy::ProgramOrder;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "usage: mirsched <dag.json> [--sched=program|list|"
                     "list-pressure] [--regs=K] [--config=cost.json] [--csv]\n";
        return 2;
    }

    std::string dagPath = argv[1];
    std::string policyStr = "program";
    std::string configPath;
    int regs = 16;
    bool csv = false;

    for (int i = 2; i < argc; ++i) {
        const std::string arg = argv[i];
        if (!argValue(arg, "--sched").empty()) policyStr = argValue(arg, "--sched");
        else if (!argValue(arg, "--config").empty())
            configPath = argValue(arg, "--config");
        else if (!argValue(arg, "--regs").empty())
            regs = std::stoi(argValue(arg, "--regs"));
        else if (arg == "--csv") csv = true;
    }

    mirsched::CostModel cm =
        configPath.empty() ? mirsched::CostModel{}
                           : mirsched::CostModel::fromJsonFile(configPath);

    mirsched::Dag dag;
    try {
        dag = mirsched::Dag::fromJsonFile(dagPath);
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }

    dag.addMemoryEdges();
    dag.applyLatencies(cm);
    dag.computeCriticalPath();  // no-op until you implement it (Day 3)

    const mirsched::SchedPolicy policy = parsePolicy(policyStr);
    const mirsched::ListScheduler scheduler(dag, cm);
    const mirsched::ScheduleResult sched = scheduler.run(policy);

    mirsched::AllocResult alloc;
    if (sched.valid) {
        const auto intervals = mirsched::buildLiveIntervals(dag, sched);
        alloc = mirsched::LinearScan(regs).run(intervals, cm);
    }

    if (csv) {
        std::cout << dag.function << ',' << policyStr << ',' << regs << ','
                  << (sched.valid ? std::to_string(sched.length) : "NA") << ','
                  << (sched.valid ? std::to_string(alloc.peakPressure) : "NA")
                  << ','
                  << (alloc.valid ? std::to_string(alloc.spills) : "NA") << ','
                  << (sched.valid ? std::to_string(alloc.estOccupancy) : "NA")
                  << ',' << (sched.valid && alloc.valid ? "1" : "0") << '\n';
        return 0;
    }

    std::cout << "function      : " << dag.function << "\n"
              << "nodes / edges : " << dag.size() << " / " << dag.edges.size()
              << "\n"
              << "policy        : " << policyStr << "\n";
    if (!sched.valid) {
        std::cout << "schedule      : NOT IMPLEMENTED (see ListScheduler TODO)\n";
        return 0;
    }
    std::cout << "sched length  : " << sched.length << " cycles\n"
              << "peak pressure : " << alloc.peakPressure << "\n"
              << "est occupancy : " << alloc.estOccupancy << "\n";
    if (!alloc.valid) {
        std::cout << "spills        : NOT IMPLEMENTED (see LinearScan TODO)\n";
    } else {
        std::cout << "spills        : " << alloc.spills << "\n";
    }
    return 0;
}

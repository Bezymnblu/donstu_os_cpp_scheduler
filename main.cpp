#include <iostream>
#include <vector>
#include <memory>
#include <iomanip>
#include "process.h"
#include "simulator.h"
#include "fcfs.h"
#include "sjf.h"
#include "srtn.h"
#include "rr.h"
#include "priority.h"
#include "mlfq.h"
#include "hrrn.h"
#include "testsets.h"

void printResult(const SimResult& r) {
    std::cout << std::left << std::setw(30) << r.algorithm << " | "
              << "wait=" << std::fixed << std::setprecision(2) << std::setw(7) << r.avgWaiting << " | "
              << "turn=" << std::setw(7) << r.avgTurnaround << " | "
              << "resp=" << std::setw(7) << r.avgResponse << " | "
              << "CPU=" << std::setw(7) << r.cpuUtilization << "%\n";
}

void printGanttWithNames(const SimResult& r, Scheduler& sched) {
    std::cout << "Gantt (" << r.algorithm << "):\n";
    for (const auto& interval : r.gantt) {
        int pid = interval.first;
        std::uint64_t start = interval.second.first;
        std::uint64_t end = interval.second.second;
        if (pid == -1) {
            std::cout << "  [" << start << "-" << end << ") IDLE\n";
        } else if (pid == -2) {
            std::cout << "  [" << start << "-" << end << ") CS\n";
        } else {
            Process* p = sched.find(pid);
            std::string name = p ? p->name : ("P" + std::to_string(pid));
            std::cout << "  [" << start << "-" << end << ") " << name << "\n";
        }
    }
}

std::vector<Process> makeTestSet() {
    std::vector<Process> procs;
    auto add = [&](int pid, std::string name, std::uint64_t arrival, std::uint64_t burst, int priority) {
        Process p;
        p.pid = pid; p.name = name; p.arrivalTime = arrival;
        p.burstTime = burst; p.remainingTime = burst;
        p.priority = priority; p.dynamicPriority = priority;
        procs.push_back(p);
    };
    add(1, "P1", 0, 8, 3);
    add(2, "P2", 1, 4, 4);
    add(3, "P3", 2, 9, 1);
    add(4, "P4", 3, 5, 2);
    return procs;
}

int main() {
    std::cout << "Case 1: CPU-bound\n";
    {
        auto set = makeTestSet();
        FcfsScheduler fcfs(set); printResult(runSimulation(fcfs));
        SjfScheduler sjf(set); printResult(runSimulation(sjf));
        SrtnScheduler srtn(set); printResult(runSimulation(srtn));
        RrScheduler rr1(set, 1); printResult(runSimulation(rr1));
        RrScheduler rr2(set, 2); printResult(runSimulation(rr2));
        RrScheduler rr4(set, 4); printResult(runSimulation(rr4));
        PriorityScheduler p1(set, false, false); printResult(runSimulation(p1));
        PriorityScheduler p2(set, true, false); printResult(runSimulation(p2));
        PriorityScheduler p3(set, true, true); printResult(runSimulation(p3));
        MlfqScheduler mlfq(set); printResult(runSimulation(mlfq));
    }

    // Задание 4: Эксперимент с накладными расходами переключения контекста
    {
        std::cout << "\n=== Задание 4: Проверка накладных расходов в RR ===\n";
        std::cout << "Квант | wait (0) | turn (0) | wait (1) | turn (1) | CPU % (1) | overhead % (1)\n";
        std::cout << "--------------------------------------------------------------------------\n";
        
        for (std::uint64_t q : {1, 2, 4, 8}) {
            auto set0 = makeTestSet();
            RrScheduler rr0(set0, q);
            SimResult r0 = runSimulation(rr0, 100000, 0); // cost = 0
            
            auto set1 = makeTestSet();
            RrScheduler rr1(set1, q);
            SimResult r1 = runSimulation(rr1, 100000, 1); // cost = 1
            
            std::cout << std::setw(5) << q << " | "
                      << std::fixed << std::setprecision(2)
                      << std::setw(8) << r0.avgWaiting << " | "
                      << std::setw(8) << r0.avgTurnaround << " | "
                      << std::setw(8) << r1.avgWaiting << " | "
                      << std::setw(8) << r1.avgTurnaround << " | "
                      << std::setw(9) << r1.cpuUtilization << " | "
                      << std::setw(14) << r1.overheadPercent << "\n";
        }
    }

    return 0;
}

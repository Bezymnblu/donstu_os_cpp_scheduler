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

// Новая умная функция вывода диаграммы Ганта, которая знает имена процессов
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

    {
        auto set = makeTestSet();
        RrScheduler rr(set, 2);
        runSimulation(rr);
        std::cout << "\nПо процессам, RR (q=2):\n";
        printProcessTable(rr.processes());
    }

    {
        auto set = makeTestSet();
        saveSet("set_basic.txt", set);
        std::vector<Process> loaded;
        if (loadSet("set_basic.txt", loaded)) {
            std::cout << "\nПроверка сохранения/загрузки:\n";
            FcfsScheduler a(set), b(loaded);
            printResult(runSimulation(a));
            printResult(runSimulation(b));
        }
    }

    {
        std::cout << "\n=== Сравнение SJF и HRRN ===\n";
        auto makeHrrnSet = []() {
            std::vector<Process> procs;
            auto add = [&](int pid, std::string name, std::uint64_t arr, std::uint64_t burst) {
                Process p; p.pid = pid; p.name = name; p.arrivalTime = arr;
                p.burstTime = burst; p.remainingTime = burst; p.priority = 1; p.dynamicPriority = 1;
                procs.push_back(p);
            };
            add(1, "A", 0, 4);
            add(2, "L", 1, 6);
            add(3, "S1", 4, 2);
            add(4, "S2", 5, 1);
            return procs;
        };

        auto setSjf = makeHrrnSet();
        SjfScheduler sjf(setSjf);
        std::cout << "--- Вывод SJF ---:\n";
        SimResult rSjf = runSimulation(sjf);
        printResult(rSjf);
        printGanttWithNames(rSjf, sjf);

        auto setHrrn = makeHrrnSet();
        HrrnScheduler hrrn(setHrrn);
        std::cout << "\n--- Вывод HRRN ---:\n";
        SimResult rHrrn = runSimulation(hrrn);
        printResult(rHrrn);
        printGanttWithNames(rHrrn, hrrn);
    }

    return 0;
}

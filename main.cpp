#include <iostream>
#include <vector>
#include <memory>
#include <iomanip>
#include <functional>
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

void printResultBasic(const SimResult& r) {
    std::cout << std::left << std::setw(30) << r.algorithm << " | "
              << "wait=" << std::fixed << std::setprecision(2) << std::setw(7) << r.avgWaiting << " | "
              << "turn=" << std::setw(7) << r.avgTurnaround << " | "
              << "resp=" << std::setw(7) << r.avgResponse << " | "
              << "CPU=" << std::setw(7) << r.cpuUtilization << "%\n";
}

void printMultiCoreLine(std::string algo, std::string cores, const SimResult& r) {
    std::cout << std::left << std::setw(15) << algo << " | "
              << std::setw(6) << cores << " | "
              << std::fixed << std::setprecision(2)
              << std::right << std::setw(6) << r.avgWaiting << " | "
              << std::setw(7) << r.avgTurnaround << " | "
              << std::setw(6) << r.avgResponse << " | "
              << std::setw(7) << r.cpuUtilization << "% | "
              << std::left << r.totalTicks << "\n";
}

std::vector<Process> makeTestSet() {
    std::vector<Process> procs;
    auto add = [&](int pid, std::string name, std::uint64_t arrival, std::uint64_t burst, int priority) {
        Process p; p.pid = pid; p.name = name; p.arrivalTime = arrival;
        p.burstTime = burst; p.remainingTime = burst; p.priority = priority; p.dynamicPriority = priority;
        procs.push_back(p);
    };
    add(1, "P1", 0, 8, 3); add(2, "P2", 1, 4, 4); add(3, "P3", 2, 9, 1); add(4, "P4", 3, 5, 2);
    return procs;
}

std::vector<Process> makeConvoySet() {
    std::vector<Process> procs;
    auto add = [&](int pid, const std::string& name, std::uint64_t arrival, std::uint64_t burst, int priority, std::vector<IoBlock> io = {}) {
        Process p; p.pid = pid; p.name = name; p.arrivalTime = arrival; p.burstTime = burst; p.remainingTime = burst;
        p.priority = priority; p.dynamicPriority = priority; p.ioBlocks = std::move(io); procs.push_back(p);
    };
    add(1, "CPU1", 0, 20, 2);
    add(2, "IO1",  1,  6, 1, {{1, 4}, {2, 4}, {3, 4}, {4, 4}, {5, 4}});
    add(3, "IO2",  2,  6, 1, {{1, 4}, {2, 4}, {3, 4}, {4, 4}, {5, 4}});
    add(4, "CPU2", 3, 12, 3);
    return procs;
}

int main() {
    std::cout << "Case 1: CPU-bound\n";
    {
        auto set = makeTestSet();
        FcfsScheduler fcfs(set); printResultBasic(runSimulation(fcfs));
        SjfScheduler sjf(set); printResultBasic(runSimulation(sjf));
        SrtnScheduler srtn(set); printResultBasic(runSimulation(srtn));
        RrScheduler rr1(set, 1); printResultBasic(runSimulation(rr1));
        RrScheduler rr2(set, 2); printResultBasic(runSimulation(rr2));
        RrScheduler rr4(set, 4); printResultBasic(runSimulation(rr4));
        PriorityScheduler p1(set, false, false); printResultBasic(runSimulation(p1));
        PriorityScheduler p2(set, true, false); printResultBasic(runSimulation(p2));
        PriorityScheduler p3(set, true, true); printResultBasic(runSimulation(p3));
        MlfqScheduler mlfq(set); printResultBasic(runSimulation(mlfq));
    }

    // Задание 4
    {
        std::cout << "\n=== Задание 4: Проверка накладных расходов в RR ===\n";
        std::cout << "Квант | wait (0) | turn (0) | wait (1) | turn (1) | CPU % (1) | overhead % (1)\n";
        std::cout << "--------------------------------------------------------------------------\n";
        for (std::uint64_t q : {1, 2, 4, 8}) {
            auto set0 = makeTestSet(); RrScheduler rr0(set0, q); SimResult r0 = runSimulation(rr0, 100000, 0);
            auto set1 = makeTestSet(); RrScheduler rr1(set1, q); SimResult r1 = runSimulation(rr1, 100000, 1);
            std::cout << std::setw(5) << q << " | " << std::fixed << std::setprecision(2)
                      << std::setw(8) << r0.avgWaiting << " | " << std::setw(8) << r0.avgTurnaround << " | "
                      << std::setw(8) << r1.avgWaiting << " | " << std::setw(8) << r1.avgTurnaround << " | "
                      << std::setw(9) << r1.cpuUtilization << " | " << std::setw(14) << r1.overheadPercent << "\n";
        }
    }

    // Задание 5
    {
        std::cout << "\n=== Задание 5: Влияние кванта в RR ===\n";
        std::cout << "q   wait   turn   resp   CS   график переключений\n";
        std::cout << "------------------------------------------------------\n";
        for (std::uint64_t q : {1, 2, 4, 8, 16}) {
            auto set = makeTestSet(); RrScheduler rr(set, q); SimResult r = runSimulation(rr, 100000, 0);
            std::cout << std::setw(2) << q << "  " << std::fixed << std::setprecision(2)
                      << std::setw(5) << r.avgWaiting << "  " << std::setw(5) << r.avgTurnaround << "  " 
                      << std::setw(5) << r.avgResponse << "  " << std::setw(3) << r.contextSwitches << "  "
                      << std::string(r.contextSwitches, '#') << "\n";
        }
    }

    // Задание 6
    {
        std::cout << "\n=== Задание 6: Исследование эффекта конвоя ===\n";
        auto runAndPrint = [&](std::string label, auto make_sched) {
            auto set = makeConvoySet(); auto sched = make_sched(set); SimResult r = runSimulation(*sched, 100000, 0);
            std::cout << std::left << std::setw(30) << label << " | wait=" << std::fixed << std::setprecision(2) << std::setw(6) << r.avgWaiting << " | turn=" << std::setw(6) << r.avgTurnaround << "%\n";
        };
        runAndPrint("FCFS", [](auto& s) { return std::make_unique<FcfsScheduler>(s); });
        runAndPrint("SRTN", [](auto& s) { return std::make_unique<SrtnScheduler>(s); });
        runAndPrint("RR (q=4)", [](auto& s) { return std::make_unique<RrScheduler>(s, 4); });
        runAndPrint("Priority (preemptive) + aging", [](auto& s) { return std::make_unique<PriorityScheduler>(s, true, true); });
        runAndPrint("MLFQ", [](auto& s) { return std::make_unique<MlfqScheduler>(s); });
    }

    // Задание 10
    {
        std::cout << "\n=== Задание 10: Сводное сравнение среднего времени ожидания ===\n";
        using Factory = std::function<std::unique_ptr<Scheduler>(const std::vector<Process>&)>;
        std::vector<std::pair<std::string, Factory>> algos = {
            {"FCFS",       [](const auto& s){ return std::make_unique<FcfsScheduler>(s); }},
            {"SJF",        [](const auto& s){ return std::make_unique<SjfScheduler>(s); }},
            {"SRTN",       [](const auto& s){ return std::make_unique<SrtnScheduler>(s); }},
            {"HRRN",       [](const auto& s){ return std::make_unique<HrrnScheduler>(s); }},
            {"RR q=4",     [](const auto& s){ return std::make_unique<RrScheduler>(s, 4); }},
            {"Prio",       [](const auto& s){ return std::make_unique<PriorityScheduler>(s, false, false); }},
            {"Prio+aging", [](const auto& s){ return std::make_unique<PriorityScheduler>(s, true, true); }},
            {"MLFQ",       [](const auto& s){ return std::make_unique<MlfqScheduler>(s); }}
        };
        const int sizes[] = {12, 14, 16, 18, 20};
        std::cout << std::left << std::setw(12) << "Algorithm" << std::right << std::setw(9) << "set 1" << std::setw(9) << "set 2" << std::setw(9) << "set 3" << std::setw(9) << "set 4" << std::setw(9) << "set 5" << std::setw(9) << "average" << "\n";
        for (auto& [label, make] : algos) {
            std::cout << std::left << std::setw(12) << label << std::right; double sum = 0;
            for (unsigned seed = 1; seed <= 5; ++seed) {
                auto set = makeRandomSet(seed, sizes[seed - 1]); auto sched = make(set); SimResult r = runSimulation(*sched, 100000, 0);
                sum += r.avgWaiting; std::cout << std::setw(9) << std::fixed << std::setprecision(2) << r.avgWaiting;
            }
            std::cout << std::setw(9) << sum / 5 << "\n";
        }
    }

    // Задание 7: Многоядерный режим (Строго по скриншотам!)
    {
        std::cout << "\n=== Задание 7: Результаты многоядерного моделирования ===\n\n";
        std::cout << std::left << std::setw(15) << "Алгоритм" << " | "
                  << std::setw(6) << "Ядра" << " | "
                  << std::setw(6) << "wait" << " | "
                  << std::setw(7) << "turn" << " | "
                  << std::setw(6) << "resp" << " | "
                  << std::setw(8) << "CPU %" << " | "
                  << "всего тактов\n";
        std::cout << "----------------------------------------------------------------------------------\n";

        // Первая таблица на скрине: FCFS
        auto f1 = makeTestSet(); FcfsScheduler fcfs1(f1); printMultiCoreLine("FCFS", "1", runSimulationMulti(fcfs1, 1));
        auto f2 = makeTestSet(); FcfsScheduler fcfs2(f2); printMultiCoreLine("FCFS", "2", runSimulationMulti(fcfs2, 2));
        auto f4 = makeTestSet(); FcfsScheduler fcfs4(f4); printMultiCoreLine("FCFS", "4", runSimulationMulti(fcfs4, 4));
        
        std::cout << "----------------------------------------------------------------------------------\n";

        // Вторая таблица на скрине: SJF + SRTN
        auto s1 = makeTestSet(); SjfScheduler sjf1(s1);   printMultiCoreLine("SJF", "1", runSimulationMulti(sjf1, 1));
        auto s2 = makeTestSet(); SjfScheduler sjf2(s2);   printMultiCoreLine("SJF", "2", runSimulationMulti(sjf2, 2));
        auto s4 = makeTestSet(); SjfScheduler sjf4(s4);   printMultiCoreLine("SJF", "4", runSimulationMulti(sjf4, 4));
        auto sr2 = makeTestSet(); SrtnScheduler srtn2(sr2); printMultiCoreLine("SRTN", "2", runSimulationMulti(srtn2, 2));
        
        std::cout << "----------------------------------------------------------------------------------\n";
    }

    return 0;
}

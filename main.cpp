#include <iostream>
#include <vector>
#include <memory>
#include <iomanip>
#include <functional>
#include <string>
#include "process.h"
#include "simulator.h"
#include "fcfs.h"
#include "sjf.h"
#include "srtn.h"
#include "rr.h"
#include "priority.h"
#include "mlfq.h"
#include "hrrn.h"
#include "edf.h"
#include "testsets.h"

void printResultBasic(const SimResult& r) {
    std::cout << std::left << std::setw(30) << r.algorithm << " | wait=" 
              << std::fixed << std::setprecision(2) << std::setw(7) << r.avgWaiting 
              << " | turn=" << std::setw(7) << r.avgTurnaround 
              << " | resp=" << std::setw(7) << r.avgResponse 
              << " | CPU=" << std::setw(7) << r.cpuUtilization << "%\n";
}

void printMultiCoreLine(std::string algo, std::string cores, const SimResult& r) {
    std::cout << std::left << std::setw(15) << algo << " | " << std::setw(6) << cores 
              << " | " << std::fixed << std::setprecision(2) << std::right << std::setw(6) << r.avgWaiting 
              << " | " << std::setw(7) << r.avgTurnaround << " | " << std::setw(6) << r.avgResponse 
              << " | " << std::setw(7) << r.cpuUtilization << "% | " << std::left << r.totalTicks << "\n";
}

void printGanttWithNames(const SimResult& r, Scheduler& sched) {
    std::cout << "Gantt (" << r.algorithm << "):\n";
    for (const auto& interval : r.gantt) {
        int pid = interval.first;
        std::uint64_t start = interval.second.first;
        std::uint64_t end = interval.second.second;
        if (pid == -1) std::cout << "  [" << start << "-" << end << ") IDLE\n";
        else if (pid == -2) std::cout << "  [" << start << "-" << end << ") CS\n";
        else {
            Process* p = sched.find(pid);
            std::string name = p ? p->name : ("P" + std::to_string(pid));
            std::cout << "  [" << start << "-" << end << ") " << name << "\n";
        }
    }
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

    {
        auto set = makeTestSet(); RrScheduler rr(set, 2); runSimulation(rr);
        std::cout << "\nПо процессам, RR (q=2):\n"; printProcessTable(rr.processes());
    }

    {
        auto set = makeTestSet(); saveSet("set_basic.txt", set);
        std::vector<Process> loaded;
        if (loadSet("set_basic.txt", loaded)) {
            std::cout << "\nПроверка сохранения/загрузки:\n";
            FcfsScheduler a(set), b(loaded);
            printResultBasic(runSimulation(a)); printResultBasic(runSimulation(b));
        }
    }

    {
        std::cout << "\n=== Задание 3: Сравнение SJF и HRRN ===\n";
        auto makeHrrnSet = []() {
            std::vector<Process> procs;
            auto add = [&](int pid, std::string name, std::uint64_t arr, std::uint64_t burst) {
                Process p; p.pid = pid; p.name = name; p.arrivalTime = arr;
                p.burstTime = burst; p.remainingTime = burst; p.priority = 1; p.dynamicPriority = 1;
                procs.push_back(p);
            };
            add(1, "A", 0, 4); add(2, "L", 1, 6); add(3, "S1", 4, 2); add(4, "S2", 5, 1);
            return procs;
        };
        auto setSjf = makeHrrnSet(); SjfScheduler sjf(setSjf);
        std::cout << "--- Вывод SJF ---:\n"; SimResult rSjf = runSimulation(sjf);
        printResultBasic(rSjf); printGanttWithNames(rSjf, sjf);

        auto setHrrn = makeHrrnSet(); HrrnScheduler hrrn(setHrrn);
        std::cout << "\n--- Вывод HRRN ---:\n"; SimResult rHrrn = runSimulation(hrrn);
        printResultBasic(rHrrn); printGanttWithNames(rHrrn, hrrn);
    }
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

    {
        std::cout << "\n=== Задание 6: Исследование эффекта конвоя ===\n";
        auto runConvoy = [&](auto make_sched) {
            auto set = makeConvoySet(); auto sched = make_sched(set); SimResult r = runSimulation(*sched, 100000, 0);
            printResultBasic(r);
        };
        runConvoy([](auto& s) { return std::make_unique<FcfsScheduler>(s); });
        runConvoy([](auto& s) { return std::make_unique<SrtnScheduler>(s); });
        runConvoy([](auto& s) { return std::make_unique<RrScheduler>(s, 4); });
        runConvoy([](auto& s) { return std::make_unique<PriorityScheduler>(s, true, true); });
        runConvoy([](auto& s) { return std::make_unique<MlfqScheduler>(s); });
    }

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
        std::cout << "----------------------------------------------------------------------\n";
        for (auto& [label, make] : algos) {
            std::cout << std::left << std::setw(12) << label << std::right; double sum = 0;
            for (unsigned seed = 1; seed <= 5; ++seed) {
                auto set = makeRandomSet(seed, sizes[seed - 1]); auto sched = make(set); SimResult r = runSimulation(*sched, 100000, 0);
                sum += r.avgWaiting; std::cout << std::setw(9) << std::fixed << std::setprecision(2) << r.avgWaiting;
            }
            std::cout << std::setw(9) << sum / 5 << "\n";
        }
    }
    {
        std::cout << "\n=== Задание 7: Результаты многоядерного моделирования ===\n\n";
        std::cout << std::left << std::setw(15) << "Алгоритм" << " | " << std::setw(6) << "Ядра" << " | " << std::setw(6) << "wait" << " | " << std::setw(7) << "turn" << " | " << std::setw(6) << "resp" << " | " << std::setw(8) << "CPU %" << " | " << "всего тактов\n";
        std::cout << "----------------------------------------------------------------------------------\n";
        auto f1 = makeTestSet(); FcfsScheduler fcfs1(f1); printMultiCoreLine("FCFS", "1", runSimulationMulti(fcfs1, 1));
        auto f2 = makeTestSet(); FcfsScheduler fcfs2(f2); printMultiCoreLine("FCFS", "2", runSimulationMulti(fcfs2, 2));
        auto f4 = makeTestSet(); FcfsScheduler fcfs4(f4); printMultiCoreLine("FCFS", "4", runSimulationMulti(fcfs4, 4));
        std::cout << "----------------------------------------------------------------------------------\n";
        auto s1 = makeTestSet(); SjfScheduler sjf1(s1);   printMultiCoreLine("SJF", "1", runSimulationMulti(sjf1, 1));
        auto s2 = makeTestSet(); SjfScheduler sjf2(s2);   printMultiCoreLine("SJF", "2", runSimulationMulti(sjf2, 2));
        auto s4 = makeTestSet(); SjfScheduler sjf4(s4);   printMultiCoreLine("SJF", "4", runSimulationMulti(sjf4, 4));
        auto sr2 = makeTestSet(); SrtnScheduler srtn2(sr2); printMultiCoreLine("SRTN", "2", runSimulationMulti(srtn2, 2));
        std::cout << "----------------------------------------------------------------------------------\n";
    }

    {
        std::cout << "\n=== Задание 12: Планирование реального времени EDF ===\n\n";
        auto runEdf = [](const std::string& name, std::vector<Process> set) {
            EdfScheduler edf(set); SimResult r = runSimulation(edf);
            std::cout << "Набор " << name << ":";
            for (const auto& interval : r.gantt) {
                if (interval.first >= 1) {
                    std::cout << " " << name << interval.first << "[" << interval.second.first << "-" << interval.second.second << ")";
                }
            }
            std::cout << "\n";
            for (const auto& p : edf.processes()) {
                std::cout << "  " << p.name << ": finish=" << p.finishTime << " deadline=" << p.deadline 
                          << (p.finishTime <= p.deadline ? "   OK\n" : "   MISSED\n");
            }
        };
        std::vector<Process> setE;
        auto addE = [&](int id, std::string n, uint64_t a, uint64_t b, uint64_t d) {
            Process p; p.pid = id; p.name = n; p.arrivalTime = a; p.burstTime = b; p.remainingTime = b; p.deadline = d; setE.push_back(p);
        };
        addE(1, "E1", 0, 3, 7); addE(2, "E2", 1, 2, 4); addE(3, "E3", 5, 2, 9); addE(4, "E4", 7, 3, 10);
        runEdf("E", setE); std::cout << "\n";

        std::vector<Process> setF;
        auto addF = [&](int id, std::string n, uint64_t a, uint64_t b, uint64_t d) {
            Process p; p.pid = id; p.name = n; p.arrivalTime = a; p.burstTime = b; p.remainingTime = b; p.deadline = d; setF.push_back(p);
        };
        addF(1, "F1", 0, 4, 5); addF(2, "F2", 4, 3, 5); addF(3, "F3", 4, 2, 6);
        runEdf("F", setF);
    }

    return 0;
}

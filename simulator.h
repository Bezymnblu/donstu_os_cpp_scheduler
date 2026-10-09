#pragma once
#include <vector>
#include <string>
#include <utility>
#include <cstdint>
#include "scheduler.h"

struct SimResult {
    std::string algorithm;
    double avgWaiting = 0;
    double avgTurnaround = 0;
    double avgResponse = 0;
    double cpuUtilization = 0;
    std::uint64_t contextSwitches = 0;
    
    // Накладные расходы (Задание 4)
    std::uint64_t overheadTicks = 0;
    double overheadPercent = 0;

    // Общее количество тактов работы симулятора (Задание 7)
    std::uint64_t totalTicks = 0;

    // Диаграмма Ганта: пара {pid, {startTick, endTick}}
    std::vector<std::pair<int, std::pair<std::uint64_t, std::uint64_t>>> gantt;
    // Многоядерная диаграмма Ганта (Задание 7)
    std::vector<std::vector<std::pair<int, std::pair<std::uint64_t, std::uint64_t>>>> coreGantt;
};

// Одноядерный симулятор
SimResult runSimulation(Scheduler& sched, std::uint64_t maxTicks = 100000, std::uint64_t switchCost = 0);

// Многоядерный симулятор (Задание 7)
SimResult runSimulationMulti(Scheduler& sched, int cores, std::uint64_t maxTicks = 100000);

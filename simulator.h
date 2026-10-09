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
    
    // Новые поля для Задания 4
    std::uint64_t overheadTicks = 0;
    double overheadPercent = 0;

    // Упрощенная диаграмма Ганта: пара {pid, {startTick, endTick}}
    std::vector<std::pair<int, std::pair<std::uint64_t, std::uint64_t>>> gantt;
};

// Параметр switchCost по умолчанию равен 0, чтобы старые вызовы работали без изменений
SimResult runSimulation(Scheduler& sched, std::uint64_t maxTicks = 100000, std::uint64_t switchCost = 0);

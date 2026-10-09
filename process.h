#pragma once
#include <string>
#include <vector>
#include <cstdint>

enum class ProcessState {
    NEW,
    READY,
    RUNNING,
    WAITING,
    TERMINATED
};

struct IoBlock {
    std::uint64_t atTick;
    std::uint64_t duration;
};

struct Process {
    int pid = 0;
    std::string name;
    std::uint64_t arrivalTime = 0;
    std::uint64_t burstTime = 0;
    std::uint64_t remainingTime = 0;
    int priority = 0;
    int dynamicPriority = 0;
    std::uint64_t deadline = 0; 
    int queueLevel = 0; 

    // Метрики
    bool started = false; 
    std::uint64_t startTime = 0;
    std::uint64_t finishTime = 0;
    std::uint64_t waitingTime = 0;
    std::uint64_t turnaroundTime = 0;
    std::uint64_t responseTime = 0;
    std::uint64_t ioWaitTime = 0;
    
    // Поля, необходимые для симулятора и ввода-вывода
    std::uint64_t executedTicks = 0;   // Добавлено обратно
    std::uint64_t contextSwitches = 0; // Добавлено обратно

    std::vector<IoBlock> ioBlocks;
    std::size_t nextIoIndex = 0;
    std::uint64_t ioReturnTick = 0;

    bool isFinished() const { return state == ProcessState::TERMINATED; }
    ProcessState state = ProcessState::NEW;
};

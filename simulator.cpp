#include "simulator.h"
#include <algorithm>

SimResult runSimulation(Scheduler& sched, std::uint64_t maxTicks, std::uint64_t switchCost) {
    SimResult res;
    res.algorithm = sched.name();
    
    auto& procs = sched.processes();
    int currentPid = -1;
    int prevPid = -1;
    std::uint64_t busyTicks = 0;
    std::uint64_t tick = 0;
    
    std::uint64_t switchLeft = 0;
    std::uint64_t overheadTicks = 0;
    
    while (tick < maxTicks) {
        for (auto& p : procs) {
            if (p.state == ProcessState::WAITING && tick >= p.ioReturnTick) {
                p.state = ProcessState::READY;
                sched.onProcessReady(p.pid, tick);
            }
        }
        
        bool allDone = true;
        for (const auto& p : procs) {
            if (!p.isFinished()) {
                allDone = false;
                break;
            }
        }
        if (allDone) break;
        
        sched.onTick(tick);
        
        if (currentPid != -1 && switchLeft == 0) {
            if (sched.shouldPreempt(currentPid, tick)) {
                Process* p = sched.find(currentPid);
                if (p && p->state == ProcessState::RUNNING) {
                    p->state = ProcessState::READY;
                    sched.onProcessPreempted(currentPid, tick);
                    currentPid = -1;
                }
            }
        }
        
        if (currentPid == -1 && switchLeft == 0) {
            currentPid = sched.pickNext(tick);
            if (currentPid != -1) {
                Process* p = sched.find(currentPid);
                if (p) {
                    p->state = ProcessState::RUNNING;
                    if (!p->started) {
                        p->started = true;
                        p->startTime = tick;
                        p->responseTime = p->startTime - p->arrivalTime;
                    }
                }
            }
        }
        
        if (prevPid != currentPid) {
            res.contextSwitches++;
            if (switchCost > 0 && prevPid != -1) { 
                switchLeft = switchCost; 
            }
        }
        
        int ranPid = -1;
        bool overheadTick = (currentPid != -1 && switchLeft > 0);
        
        if (overheadTick) {
            switchLeft--;
            overheadTicks++;
        } else if (currentPid != -1) {
            Process* p = sched.find(currentPid);
            if (p && p->state == ProcessState::RUNNING) {
                ranPid = currentPid;
                p->remainingTime--;
                p->executedTicks++;
                busyTicks++;
                sched.onProcessRanTick(currentPid);
                
                if (p->nextIoIndex < p->ioBlocks.size() &&
                    p->executedTicks == p->ioBlocks[p->nextIoIndex].atTick) {
                    p->state = ProcessState::WAITING;
                    p->ioReturnTick = tick + 1 + p->ioBlocks[p->nextIoIndex].duration;
                    p->nextIoIndex++;
                    sched.onProcessBlocked(currentPid, tick);
                    currentPid = -1;
                }
                else if (p->remainingTime == 0) {
                    p->state = ProcessState::TERMINATED;
                    p->finishTime = tick + 1;
                    p->turnaroundTime = p->finishTime - p->arrivalTime;
                    sched.onProcessFinished(currentPid, tick + 1);
                    currentPid = -1;
                }
            }
        }
        
        for (auto& p : procs) {
            if (p.state == ProcessState::READY) {
                p.waitingTime++;
            }
        }
        
        if (overheadTick) {
            if (!res.gantt.empty() && res.gantt.back().first == -2) res.gantt.back().second.second = tick + 1;
            else res.gantt.push_back({-2, {tick, tick + 1}});
        } else if (ranPid != -1) {
            if (!res.gantt.empty() && res.gantt.back().first == ranPid) res.gantt.back().second.second = tick + 1;
            else res.gantt.push_back({ranPid, {tick, tick + 1}});
        } else {
            if (!res.gantt.empty() && res.gantt.back().first == -1) res.gantt.back().second.second = tick + 1;
            else res.gantt.push_back({-1, {tick, tick + 1}});
        }
        
        prevPid = overheadTick ? currentPid : ranPid;
        tick++;
    }
    
    double totalWait = 0, totalTurn = 0, totalResp = 0;
    for (const auto& p : procs) {
        totalWait += p.waitingTime;
        totalTurn += p.turnaroundTime;
        totalResp += p.responseTime;
    }
    
    res.avgWaiting = totalWait / procs.size();
    res.avgTurnaround = totalTurn / procs.size();
    res.avgResponse = totalResp / procs.size();
    res.overheadTicks = overheadTicks;
    res.cpuUtilization = tick > 0 ? (100.0 * busyTicks / tick) : 0.0;
    res.overheadPercent = tick > 0 ? (100.0 * overheadTicks / tick) : 0.0;
    res.totalTicks = tick;
    
    return res;
}

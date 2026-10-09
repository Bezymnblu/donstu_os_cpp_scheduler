#include "simulator.h"
#include <algorithm>

SimResult runSimulationMulti(Scheduler& sched, int cores, std::uint64_t maxTicks) {
    SimResult res;
    res.algorithm = sched.name();
    res.coreGantt.resize(cores);

    std::vector<int> current(cores, -1);
    std::vector<int> prev(cores, -1);
    std::uint64_t busyTicks = 0;
    std::uint64_t tick = 0;
    auto& procs = sched.processes();

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

        for (int c = 0; c < cores; ++c) {
            if (current[c] != -1) {
                if (sched.shouldPreempt(current[c], tick)) {
                    Process* p = sched.find(current[c]);
                    if (p && p->state == ProcessState::RUNNING) {
                        p->state = ProcessState::READY;
                        sched.onProcessPreempted(current[c], tick);
                        current[c] = -1;
                    }
                }
            }
        }

        for (int c = 0; c < cores; ++c) {
            if (current[c] == -1) {
                current[c] = sched.pickNext(tick);
                if (current[c] != -1) {
                    Process* p = sched.find(current[c]);
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
        }

        std::vector<int> ranPid(cores, -1);
        for (int c = 0; c < cores; ++c) {
            if (current[c] != -1) {
                Process* p = sched.find(current[c]);
                if (p && p->state == ProcessState::RUNNING) {
                    ranPid[c] = current[c];
                    p->remainingTime--;
                    p->executedTicks++;
                    busyTicks++;
                    sched.onProcessRanTick(current[c]);

                    if (p->nextIoIndex < p->ioBlocks.size() &&
                        p->executedTicks == p->ioBlocks[p->nextIoIndex].atTick) {
                        p->state = ProcessState::WAITING;
                        p->ioReturnTick = tick + 1 + p->ioBlocks[p->nextIoIndex].duration;
                        p->nextIoIndex++;
                        sched.onProcessBlocked(current[c], tick);
                        current[c] = -1;
                    }
                    else if (p->remainingTime == 0) {
                        p->state = ProcessState::TERMINATED;
                        p->finishTime = tick + 1;
                        p->turnaroundTime = p->finishTime - p->arrivalTime;
                        sched.onProcessFinished(current[c], tick + 1);
                        current[c] = -1;
                    }
                }
            }
        }

        for (auto& p : procs) {
            if (p.state == ProcessState::READY) {
                p.waitingTime++;
            }
        }

        for (int c = 0; c < cores; ++c) {
            if (prev[c] != current[c] && current[c] != -1) {
                res.contextSwitches++;
            }
        }

        for (int c = 0; c < cores; ++c) {
            int pid = ranPid[c];
            if (pid != -1) {
                if (!res.coreGantt[c].empty() && res.coreGantt[c].back().first == pid) res.coreGantt[c].back().second.second = tick + 1;
                else res.coreGantt[c].push_back({pid, {tick, tick + 1}});
            } else {
                if (!res.coreGantt[c].empty() && res.coreGantt[c].back().first == -1) res.coreGantt[c].back().second.second = tick + 1;
                else res.coreGantt[c].push_back({-1, {tick, tick + 1}});
            }
            prev[c] = current[c];
        }

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
    res.cpuUtilization = (tick > 0 && cores > 0) ? (100.0 * busyTicks / (tick * cores)) : 0.0;
    res.totalTicks = tick;

    return res;
}

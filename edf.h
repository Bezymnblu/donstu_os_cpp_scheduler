#pragma once
#include "scheduler.h"
#include <algorithm>
#include <vector>

class EdfScheduler : public Scheduler {
public:
    using Scheduler::Scheduler;

    std::string name() const override { return "EDF"; }

    void onTick(std::uint64_t tick) override {
        for (auto& p : processes_) {
            if (p.arrivalTime == tick && p.state == ProcessState::NEW) {
                p.state = ProcessState::READY;
                ready_.push_back(p.pid);
            }
        }
    }

    int pickNext(std::uint64_t) override {
        if (ready_.empty()) return -1;

        auto it = std::min_element(ready_.begin(), ready_.end(),
            [&](int a, int b) {
                return find(a)->deadline < find(b)->deadline;
            });

        int pid = *it;
        ready_.erase(it);
        
        Process* p = find(pid);
        if (p) p->state = ProcessState::RUNNING; // Явно переводим в RUNNING
        return pid;
    }

    bool shouldPreempt(int currentPid, std::uint64_t) override {
        if (ready_.empty()) return false;
        const Process* curr = find(currentPid);
        auto it = std::min_element(ready_.begin(), ready_.end(),
            [&](int a, int b) { return find(a)->deadline < find(b)->deadline; });
        return find(*it)->deadline < curr->deadline;
    }

    void onProcessFinished(int pid, std::uint64_t tick) override {
        Process* p = find(pid);
        if (p) {
            p->state = ProcessState::TERMINATED;
            p->finishTime = tick;
            p->turnaroundTime = p->finishTime - p->arrivalTime;
            p->responseTime = p->startTime - p->arrivalTime;
        }
    }

    void onProcessBlocked(int, std::uint64_t) override {}
    void onProcessReady(int pid, std::uint64_t) override {
        ready_.push_back(pid);
    }

private:
    std::vector<int> ready_;
};

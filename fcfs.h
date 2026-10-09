#pragma once
#include "scheduler.h"
#include <deque>

class FcfsScheduler : public Scheduler {
public:
    using Scheduler::Scheduler;

    std::string name() const override { return "FCFS"; }

    void onTick(std::uint64_t tick) override {
        for (auto& p : processes_) {
            if (p.arrivalTime == tick && p.state == ProcessState::NEW) {
                p.state = ProcessState::READY;
                queue_.push_back(p.pid);
            }
        }
    }

    int pickNext(std::uint64_t) override {
        if (queue_.empty()) return -1;
        int pid = queue_.front();
        queue_.pop_front(); // Вынимаем процесс из очереди для многоядерного режима
        return pid;
    }

    bool shouldPreempt(int, std::uint64_t) override {
        return false;
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
        queue_.push_back(pid);
    }

private:
    std::deque<int> queue_;
};

#ifndef __RAFTCPP_RAFTTIMER_HPP__
#define __RAFTCPP_RAFTTIMER_HPP__

#include <iostream>
#include <atomic>
#include <thread>
#include <chrono>
#include <random>

class RaftTimer {
public:
    RaftTimer(int32_t min_wait_ms, int32_t max_wait_ms) : randgen(min_wait_ms, max_wait_ms) {
        started.store(false);
        rst.store(false);
    }
    template<typename Fn, typename... Args>
    bool Start(Fn fn, Args... args) {
        if (started.load()) {
            return false;
        }
        else {
            started.store(true);
        }
        auto f = [&, this](std::stop_token st) {
            while (!st.stop_requested()) {
                auto wait_ms = std::chrono::milliseconds(randgen(e));
                auto start = std::chrono::steady_clock::now();
                while (!st.stop_requested() && !rst.load() && std::chrono::steady_clock::now() - start < wait_ms) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(5));
                }
                if(st.stop_requested()) {
                    break;
                }
                if(rst.load()) {
                    rst.store(false);
                    continue;
                }
                fn(args...);
            }
        };
        t = std::jthread(f);
        t.detach();
        return true;
    }
    void Reset() {
        rst.store(true);
    }
    bool Stop() {
        return t.request_stop();
    }
private:
    std::jthread t;
    std::atomic_bool started;
    std::atomic_bool rst;

    std::default_random_engine e;
    std::uniform_int_distribution<int32_t> randgen;
};

#endif
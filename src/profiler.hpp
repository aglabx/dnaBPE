#ifndef DNA_BPE_PROFILER_HPP
#define DNA_BPE_PROFILER_HPP

#include <iostream>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <bitset>
#include <stdexcept>
#include <algorithm>
#include <string>
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <fstream>
#include <unordered_set>


// Custom profiler class
class ScopedProfiler {
    std::chrono::high_resolution_clock::time_point start;
    std::string name;
    static std::unordered_map<std::string, double> timings;

public:
    ScopedProfiler(const std::string& n) : name(n), start(std::chrono::high_resolution_clock::now()) {}
    
    ~ScopedProfiler() {
        auto end = std::chrono::high_resolution_clock::now();
        double duration = std::chrono::duration<double>(end - start).count();
        timings[name] += duration;
    }

    static void printReport() {
        std::vector<std::pair<std::string, double>> sorted(timings.begin(), timings.end());
        std::sort(sorted.begin(), sorted.end(), 
                 [](const auto& a, const auto& b) { return a.second > b.second; });
                 
        std::cerr << "\nProfiling Report:\n";
        for (const auto& [name, time] : sorted) {
            std::cerr << name << ": " << time << "s\n";
        }
    }
};

#endif // DNA_BPE_PROFILER_HPP
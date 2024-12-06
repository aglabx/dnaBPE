#ifndef DNA_BPE_PROFILER_HPP
#define DNA_BPE_PROFILER_HPP

#include <chrono>
#include <string>
#include <vector>
#include <algorithm>
#include <iostream>
#include "robin_hood.h"

// Custom profiler class
class ScopedProfiler {
    std::chrono::high_resolution_clock::time_point start;
    std::string name;
    static robin_hood::unordered_flat_map<std::string, double> timings;  // Changed to robin_hood map

public:
    ScopedProfiler(const std::string& n) : name(n), start(std::chrono::high_resolution_clock::now()) {}
    
    ~ScopedProfiler() {
        auto end = std::chrono::high_resolution_clock::now();
        double duration = std::chrono::duration<double>(end - start).count();
        timings[name] += duration;
    }

    static void printReport() {
        // Pre-allocate vector for sorting
        std::vector<std::pair<std::string, double>> sorted;
        sorted.reserve(timings.size());
        
        // Convert robin_hood::pair to std::pair explicitly
        for (const auto& pair : timings) {
            sorted.emplace_back(pair.first, pair.second);
        }
        
        // Sort by time descending
        std::sort(sorted.begin(), sorted.end(), 
                 [](const auto& a, const auto& b) { return a.second > b.second; });
                 
        // Print report
        std::cerr << "\nProfiling Report:\n";
        for (const auto& [name, time] : sorted) {
            std::cerr << name << ": " << time << "s\n";
        }
    }
};

#endif // DNA_BPE_PROFILER_HPP
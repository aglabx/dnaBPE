#include "profiler.hpp"

robin_hood::unordered_flat_map<std::string, double> ScopedProfiler::timings;

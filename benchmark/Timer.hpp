/*
 $Id$ 

 Benchmark stuff:  From src/cli/RaptorQ.cpp
*/

#pragma once
#include <chrono>

//-------------------------------------------------------------------------
class Timer {
public:
    Timer() {}
    Timer (const Timer&) = delete;
    Timer& operator= (const Timer&) = delete;
    Timer (Timer&&) = delete;
    Timer& operator= (Timer&&) = delete;
    void start()
        { t0 = std::chrono::high_resolution_clock::now(); }
    std::chrono::microseconds stop ()
    {
        auto t1 = std::chrono::high_resolution_clock::now();
        auto diff = t1 - t0;
        return std::chrono::duration_cast<std::chrono::microseconds> (diff);
    }
private:
    std::chrono::time_point<std::chrono::high_resolution_clock> t0;
};
//-------------------------------------------------------------------------

/*
 $Id$ 

 Benchmark stuff:  From src/cli/RaptorQ.cpp
*/

#pragma once
#include <chrono>
#include <string>
#include <sstream>
#include <iomanip>

//-------------------------------------------------------------------------
class Timer {
public:
    Timer(int precision = 2) { this->precision = precision; }
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
    std::string stop_sec()
	{
		std::stringstream stream;
		stream << std::fixed << std::setprecision(precision) << stop().count()/1'000'000.;
		return stream.str();
	}

private:
    std::chrono::time_point<std::chrono::high_resolution_clock> t0;
	int precision;
};
//-------------------------------------------------------------------------

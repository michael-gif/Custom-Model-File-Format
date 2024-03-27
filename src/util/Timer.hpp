#ifndef SRC_UTIL_TIMER_HPP_
#define SRC_UTIL_TIMER_HPP_

#include <iostream>
#include <chrono>

class Timer {
public:
	static std::chrono::steady_clock::time_point begin()
	{
		return std::chrono::high_resolution_clock::now();
	}
	static void end(std::chrono::steady_clock::time_point start, std::string text)
	{
	auto stop = std::chrono::high_resolution_clock::now();
        auto durationNano = std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start).count();
        if (durationNano > 1000000000) {
            std::cout << text << (durationNano / 1000000000) << " s, " << (durationNano / 1000000) << " ms" << std::endl;
        }
        else if (durationNano > 1000000) {
            std::cout << text << (durationNano / 1000000) << " ms, " << (durationNano / 1000) << " µs" << std::endl;
        }
        else {
            std::cout << text << (durationNano / 1000) << " µs, " << durationNano << " ns" << std::endl;
        }
	}
};

#endif
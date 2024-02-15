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
        auto durationSecond = std::chrono::duration_cast<std::chrono::seconds>(stop - start);
        auto durationMilli = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
        auto durationMicro = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
        if (durationMilli.count() > 1000) {
            std::cout << text << durationSecond.count() << "s, " << durationMilli.count() << "ms\n";
        }
        else {
            std::cout << text << durationMilli.count() << "ms, " << durationMicro.count() << "us\n";
        }
        std::flush(std::cout);
	}
};

#endif
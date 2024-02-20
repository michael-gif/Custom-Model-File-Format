#ifndef SRC_UTIL_PROGRESSBAR_HPP_
#define SRC_UTIL_PROGRESSBAR_HPP_

#include <iostream>

class ProgressBar {
private:
	int maxValue;
	int currentCompletionValue = 0;
	bool complete = false;
	const char progressChars[40] = {
		'.', '.', '.', '1',
		'.', '.', '.', '2',
		'.', '.', '.', '3',
		'.', '.', '.', '4',
		'.', '.', '.', '5',
		'.', '.', '.', '6',
		'.', '.', '.', '7',
		'.', '.', '.', '8',
		'.', '.', '.', '9',
		'.', '.', '.', '1'
	};
public:
	ProgressBar(int maxValue) {
		this->maxValue = maxValue;
	}

	void start() {
		std::cout << "0";
	}

	void updateProgress(int newValue) {
		if (complete) return;
		float fraction = (float)newValue / (float)maxValue;
		int completion = (int)((fraction * 40.0) + 0.5);
		int diff = completion - currentCompletionValue;
		if (diff > 0) {
			for (int i = 0; i < diff; ++i) {
				std::cout << progressChars[currentCompletionValue + i];
			}
			currentCompletionValue = completion;
			if (currentCompletionValue == 40) {
				std::cout << "0" << std::endl;
				complete = true;
			}
		}
	}
};

#endif
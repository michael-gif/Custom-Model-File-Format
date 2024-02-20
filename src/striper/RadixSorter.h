#ifndef SRC_STRIPER_RADIXSORTER_H_
#define SRC_STRIPER_RADIXSORTER_H_

#include <iostream>
#include <vector>

class RadixSorter {
private:
	std::vector<uint8_t> singleDigits;
	int divisors[10] = {
		1,
		10,
		100,
		1000,
		10000,
		100000,
		1000000,
		10000000,
		100000000,
		1000000000
	};
	int countArray[10];
	void countSort(std::vector<uint16_t>& unsortedNumbers, std::vector<int>& inputIndices, std::vector<int>& outputIndices, int digit);
public:
	void sortFast(std::vector<uint16_t>& inputArray, std::vector<int>& sortedIndices);
	void sortFast(std::vector<uint16_t>& inputArray, std::vector<int>& inputIndices, std::vector<int>& sortedIndices);
	void sortRadix(std::vector<uint16_t>& inputArray, std::vector<int>& sortedIndices);
	void sortRadix(std::vector<uint16_t>& inputArray, std::vector<int>& inputIndices, std::vector<int>& sortedIndices);
};

#endif
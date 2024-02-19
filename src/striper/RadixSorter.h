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
	void countSort(int numElements, uint16_t* unsortedNumbers, int* inputIndices, int* outputIndices, int digit);
public:
	void sort(int edgeCount, uint16_t* inputArray, int* sortedIndices);
	void sort(int edgeCount, uint16_t* inputArray, int* inputIndices, int* sortedIndices);
};

#endif
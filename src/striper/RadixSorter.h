#ifndef RADIXSORTER_H_
#define RADIXSORTER_H_

#include <iostream>
#include <vector>

class RadixSorter {
private:
	std::vector<uint8_t> singleDigits;
	std::vector<int> countArray;
	int* countsPtr = countArray.data();
	void countSort(int numElements, uint16_t* unsortedNumbers, int* inputIndices, int* outputIndices, int digit);
public:
	RadixSorter() : countArray(10, 0) {};

	void sort(std::vector<uint16_t>& unsortedNumbers, std::vector<int>& sortedIndices);
	void sort(std::vector<uint16_t>& inputArray, std::vector<int>& outputIndices, std::vector<int>& startingIndices);
};

#endif
#include <iostream>
#include <vector>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <striper/RadixSorter.h>
#include <util/Timer.hpp>

void RadixSorter::sort(std::vector<uint16_t>& inputArray, std::vector<int>& outputIndices)
{
	uint16_t maxValue = *std::max_element(inputArray.begin(), inputArray.end());
	int digit = 0;
	while (maxValue != 0) {
		maxValue = maxValue / 10;
		++digit;
	}

	int edgeCount = inputArray.size();
	std::vector<int> inputIndices(edgeCount);
	std::iota(inputIndices.begin(), inputIndices.end(), 0);

	uint16_t* inputPtr = inputArray.data();
	int* inputIndicesPtr = inputIndices.data();
	int* outputIndicesPtr = outputIndices.data();
	int* tmpPtr;

	for (int i = 0; i < digit; i++) {
		countSort(edgeCount, inputPtr, inputIndicesPtr, outputIndicesPtr, i + 1);
		tmpPtr = inputIndicesPtr;
		inputIndicesPtr = outputIndicesPtr;
		outputIndicesPtr = tmpPtr;
	}
}

void RadixSorter::sort(std::vector<uint16_t>& inputArray, std::vector<int>& outputIndices, std::vector<int>& startingIndices)
{
	uint16_t maxValue = *std::max_element(inputArray.begin(), inputArray.end());
	int digit = 0;
	while (maxValue != 0) {
		maxValue = maxValue / 10;
		++digit;
	}

	int edgeCount = inputArray.size();

	uint16_t* inputPtr = inputArray.data();
	int* inputIndicesPtr = startingIndices.data();
	int* outputIndicesPtr = outputIndices.data();
	int* tmpPtr;
	for (int i = 0; i < digit; i++) {
		countSort(edgeCount, inputPtr, inputIndicesPtr, outputIndicesPtr, i + 1);
		tmpPtr = inputIndicesPtr;
		inputIndicesPtr = outputIndicesPtr;
		outputIndicesPtr = tmpPtr;
	}
}

/// <summary>
/// Sort the unsortedNumbers and produce an array of sorted indexes into the unsortedNumbers array
/// </summary>
/// <param name="unsortedNumbers"></param>
/// <param name="sortedIndices"></param>
void RadixSorter::countSort(int numElements, uint16_t* inputArray, int* inputIndices, int* outputIndices, int digit)
{
	singleDigits.resize(numElements);
	uint8_t* singlePtr = singleDigits.data();
	int divisor = std::pow(10.0, digit) / 10;
	for (int i = 0; i < numElements; i++) {
		singlePtr[i] = (inputArray[inputIndices[i]] / divisor) % 10;
	}
	
	int numCounts = 10;
	std::fill(countArray.begin(), countArray.end(), 0);

	for (int i = 0; i < numElements; i++) {
		countsPtr[singlePtr[i]]++;
	}
	for (int i = 1; i < numCounts; i++) {
		countsPtr[i] += countsPtr[i - 1];
	}
	for (int i = numElements - 1; i >= 0; i--) {
		uint8_t element = singlePtr[i];
		outputIndices[countsPtr[element] - 1] = inputIndices[i];
		countsPtr[element]--;
	}
}
#include <iostream>
#include <vector>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <striper/RadixSorter.h>
#include <util/Timer.hpp>

void RadixSorter::sort(int edgeCount, uint16_t* inputArray, int* outputIndices)
{
	uint16_t maxValue = *std::max_element(inputArray, inputArray + edgeCount);
	int digit = 0;
	while (maxValue != 0) {
		maxValue = maxValue / 10;
		++digit;
	}
	int* inputIndices = new int[edgeCount];
	for (int i = 0; i < edgeCount; i++) {
		inputIndices[i] = i;
	}
	for (int i = 0; i < digit; i++) {
		countSort(edgeCount, inputArray, inputIndices, outputIndices, i + 1);
		if (i == digit - 1) break;
		memcpy(inputIndices, outputIndices, edgeCount * 4);
		memset(outputIndices, 0, edgeCount * 4);

	}
	delete[] inputIndices;
}

void RadixSorter::sort(int edgeCount, uint16_t* inputArray, int* inputIndices, int* outputIndices)
{
	uint16_t maxValue = *std::max_element(inputArray, inputArray + edgeCount);
	int digit = 0;
	while (maxValue != 0) {
		maxValue = maxValue / 10;
		++digit;
	}
	for (int i = 0; i < digit; i++) {
		countSort(edgeCount, inputArray, inputIndices, outputIndices, i + 1);
		if (i == digit - 1) break;
		memcpy(inputIndices, outputIndices, edgeCount * 4);
		memset(outputIndices, 0, edgeCount * 4);
	}
}

/// <summary>
/// Sort the unsortedNumbers and produce an array of sorted indexes into the unsortedNumbers array
/// </summary>
/// <param name="unsortedNumbers"></param>
/// <param name="sortedIndices"></param>
void RadixSorter::countSort(int numElements, uint16_t* inputArray, int* inputIndices, int* outputIndices, int digit)
{
	singleDigits.reserve(numElements);
	uint8_t* singlePtr = singleDigits.data();
	int divisor = divisors[digit - 1];
	for (int i = 0; i < numElements; i++) {
		singlePtr[i] = (inputArray[inputIndices[i]] / divisor) % 10;
	}
	memset(countArray, 0, 40); // reset counts array

	// create counts
	for (int i = 0; i < numElements; i++) {
		countArray[singlePtr[i]]++;
	}

	// create offsets
	for (int i = 1; i < 10; i++) {
		countArray[i] += countArray[i - 1];
	}

	// get sorted values
	for (int i = numElements - 1; i >= 0; i--) {
		uint8_t element = singlePtr[i];
		outputIndices[countArray[element] - 1] = inputIndices[i];
		countArray[element]--;
	}
}
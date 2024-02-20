#include <iostream>
#include <numeric>
#include <vector>
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
	int numElements = (int)inputArray.size();
	std::vector<int> inputIndices(numElements);
	std::iota(inputIndices.begin(), inputIndices.end(), 0);
	for (int i = 0; i < digit; i++) {
		countSort(inputArray, inputIndices, outputIndices, i + 1);
		if (i == digit - 1) break;
		memcpy(inputIndices.data(), outputIndices.data(), numElements * 4);
		memset(outputIndices.data(), 0, numElements * 4);
	}
}

void RadixSorter::sort(std::vector<uint16_t>& inputArray, std::vector<int>& inputIndices, std::vector<int>& outputIndices)
{
	uint16_t maxValue = *std::max_element(inputArray.begin(), inputArray.end());
	int digit = 0;
	while (maxValue != 0) {
		maxValue = maxValue / 10;
		++digit;
	}
	int numElements = (int)inputArray.size();
	for (int i = 0; i < digit; i++) {
		countSort(inputArray, inputIndices, outputIndices, i + 1);
		if (i == digit - 1) break;
		memcpy(inputIndices.data(), outputIndices.data(), numElements * 4);
		memset(outputIndices.data(), 0, numElements * 4);
	}
}

/// <summary>
/// Sort the unsortedNumbers and produce an array of sorted indexes into the unsortedNumbers array
/// </summary>
/// <param name="unsortedNumbers"></param>
/// <param name="sortedIndices"></param>
void RadixSorter::countSort(std::vector<uint16_t>& inputArray, std::vector<int>& inputIndices, std::vector<int>& outputIndices, int digit)
{
	int numElements = (int)inputArray.size();
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
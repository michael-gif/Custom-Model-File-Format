#include <iostream>
#include <numeric>
#include <meshstriper/Sorter.h>
#include <util/Timer.hpp>

void Sorter::sortFast(std::vector<uint16_t>& inputArray, std::vector<int>& outputIndices, int* memoryUsage)
{
	uint16_t maxValue = *std::max_element(inputArray.begin(), inputArray.end());
	int numElements = (int)inputArray.size();

	if (memoryUsage != nullptr) {
		*memoryUsage = (maxValue + 1) * sizeof(int);
		*memoryUsage += numElements * sizeof(int);
	}

	std::vector<int> counts(maxValue + 1);
	uint16_t* inputPtr = inputArray.data();
	int* countsPtr = counts.data();

	// create counts
	for (int i = 0; i < numElements; i++) {
		countsPtr[inputPtr[i]]++;
	}

	// create offsets
	for (int i = 1; i < counts.size(); i++) {
		countsPtr[i] += countsPtr[i - 1];
	}

	// get sorted values
	std::vector<int> inputIndices(numElements);
	std::iota(inputIndices.begin(), inputIndices.end(), 0);
	int* inputIndicesPtr = inputIndices.data();
	int* outputPtr = outputIndices.data();
	for (int i = numElements - 1; i >= 0; i--) {
		int element = inputPtr[i];
		outputPtr[countsPtr[element] - 1] = inputIndicesPtr[i];
		countsPtr[element]--;
	}
}

void Sorter::sortFast(std::vector<uint16_t>& inputArray, std::vector<int>& inputIndices, std::vector<int>& outputIndices, int* memoryUsage)
{
	uint16_t maxValue = *std::max_element(inputArray.begin(), inputArray.end());
	int numElements = (int)inputArray.size();

	if (memoryUsage != nullptr) {
		*memoryUsage = (maxValue + 1) * sizeof(int);
		*memoryUsage += numElements * sizeof(int);
	}

	std::vector<int> counts(maxValue + 1);
	uint16_t* inputPtr = inputArray.data();
	int* countsPtr = counts.data();
	int* inputIndicesPtr = inputIndices.data();

	// create counts
	for (int i = 0; i < numElements; i++) {
		countsPtr[inputPtr[i]]++;
	}

	// create offsets
	for (int i = 1; i < counts.size(); i++) {
		countsPtr[i] += countsPtr[i - 1];
	}

	// get sorted values
	int* outputPtr = outputIndices.data();
	for (int i = numElements - 1; i >= 0; i--) {
		int element = inputPtr[inputIndicesPtr[i]];
		outputPtr[countsPtr[element] - 1] = inputIndicesPtr[i];
		countsPtr[element]--;
	}
}

void Sorter::sortRadix(std::vector<uint16_t>& inputArray, std::vector<int>& outputIndices, int* memoryUsage)
{
	uint16_t maxValue = *std::max_element(inputArray.begin(), inputArray.end());
	int digit = 0;
	while (maxValue != 0) {
		maxValue = maxValue / 10;
		++digit;
	}
	int numElements = (int)inputArray.size();
	std::vector<int> inputIndices(numElements);

	if (memoryUsage != nullptr) {
		*memoryUsage = numElements * sizeof(int); // inputIndices
		*memoryUsage += numElements * sizeof(uint8_t); // singleDigits
		*memoryUsage += 40; // countArray
	}

	std::iota(inputIndices.begin(), inputIndices.end(), 0);
	for (int i = 0; i < digit; i++) {
		countSort(inputArray, inputIndices, outputIndices, i + 1);
		if (i == digit - 1) break;
		memcpy(inputIndices.data(), outputIndices.data(), numElements * 4);
		memset(outputIndices.data(), 0, numElements * 4);
	}
}

void Sorter::sortRadix(std::vector<uint16_t>& inputArray, std::vector<int>& inputIndices, std::vector<int>& outputIndices, int* memoryUsage)
{
	uint16_t maxValue = *std::max_element(inputArray.begin(), inputArray.end());
	int digit = 0;
	while (maxValue != 0) {
		maxValue = maxValue / 10;
		++digit;
	}
	int numElements = (int)inputArray.size();

	if (memoryUsage != nullptr) {
		*memoryUsage += numElements * sizeof(uint8_t); // singleDigits
		*memoryUsage += 40; // countArray
	}

	for (int i = 0; i < digit; i++) {
		countSort(inputArray, inputIndices, outputIndices, i + 1);
		if (i == digit - 1) break;
		memcpy(inputIndices.data(), outputIndices.data(), numElements * 4);
		memset(outputIndices.data(), 0, numElements * 4);
	}
}

void Sorter::countSort(std::vector<uint16_t>& inputArray, std::vector<int>& inputIndices, std::vector<int>& outputIndices, int digit)
{
	int numElements = (int)inputArray.size();
	singleDigits.reserve(numElements);
	uint8_t* singlePtr = singleDigits.data();
	int* inputPtr = inputIndices.data();
	int* outputPtr = outputIndices.data();
	int divisor = divisors[digit - 1];
	for (int i = 0; i < numElements; i++) {
		singlePtr[i] = (inputArray[inputPtr[i]] / divisor) % 10;
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
		outputPtr[countArray[element] - 1] = inputPtr[i];
		countArray[element]--;
	}
}
#ifndef SRC_MESHSTRIPER_SORTER_H_
#define SRC_MESHSTRIPER_SORTER_H_

#include <iostream>
#include <vector>

class Sorter {
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

	/// <summary>
	/// Sort inputArray using count sort/bucket sort.
	/// </summary>
	/// <param name="unsortedNumbers"></param>
	/// <param name="sortedIndices"></param>
	void countSort(std::vector<uint16_t>& unsortedNumbers, std::vector<int>& inputIndices, std::vector<int>& outputIndices, int digit);
public:
	/// <summary>
	/// <para/>Sort an array of uint16_t's using count sort/bucket sort. Indexes are stored in outputIndices.
	/// <para/>To get sorted results, iterate through outputIndices, using each element as an index into inputArray
	/// </summary>
	/// <param name="inputArray"></param>
	/// <param name="outputIndices"></param>
	void sortFast(std::vector<uint16_t>& inputArray, std::vector<int>& sortedIndices);

	/// <summary>
	/// <para/>Sort an array of uint16_t's using count sort/bucket sort. Indexes are stored in outputIndices.
	/// <para/>One can provide an array of input indices to influence the output indices. Useful for multi sorting.
	/// <para/>To get sorted results, iterate through outputIndices, using each element as an index into inputArray
	/// </summary>
	/// <param name="inputArray"></param>
	/// <param name="outputIndices"></param>
	void sortFast(std::vector<uint16_t>& inputArray, std::vector<int>& inputIndices, std::vector<int>& sortedIndices);

	/// <summary>
	/// <para/>Sort an array of uint16_t's using radix sort. Indexes are stored in outputIndices.
	/// <para/>To get sorted results, iterate through outputIndices, using each element as an index into inputArray
	/// </summary>
	/// <param name="inputArray"></param>
	/// <param name="outputIndices"></param>
	void sortRadix(std::vector<uint16_t>& inputArray, std::vector<int>& sortedIndices);

	/// <summary>
	/// <para/>Sort an array of uint16_t's using radix sort. Indexes are stored in outputIndices.
	/// <para/>One can provide an array of input indices to influence the output indices. Useful for multi sorting.
	/// <para/>To get sorted results, iterate through outputIndices, using each element as an index into inputArray 
	/// </summary>
	/// <param name="inputArray"></param>
	/// <param name="inputIndices"></param>
	/// <param name="outputIndices"></param>
	void sortRadix(std::vector<uint16_t>& inputArray, std::vector<int>& inputIndices, std::vector<int>& sortedIndices);
};

#endif
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
	/// <para/>Sort inputArray using count sort/bucket sort.
	/// </summary>
	/// <param name="unsortedNumbers"> - unsorted input array</param>
	/// <param name="inputIndices"> - array of indices to use during sort, useful for multisort</param>
	/// <param name="sortedIndices"> - array of indices to unsorted input array, used to produce sorted array</param>
	/// <param name="digit">- which base 10 digit to sort using</param>
	void countSort(std::vector<uint16_t>& unsortedNumbers, std::vector<int>& inputIndices, std::vector<int>& outputIndices, int digit);
public:
	/// <summary>
	/// <para/>Sort an array of uint16_t's using count sort/bucket sort. Indexes are stored in outputIndices.
	/// <para/>To get sorted results, iterate through outputIndices, using each element as an index into inputArray
	/// <para/>Uses more memory than radix sort. Memory usage is outputed to memoryUsage pointer
	/// </summary>
	/// <param name="inputArray"> - unsorted input array</param>
	/// <param name="sortedIndices"> - array of indices to unsorted input array, used to produce sorted array</param>
	/// <param name="memoryUsage"> - how much memory is created and used during the sort</param>
	void sortFast(std::vector<uint16_t>& inputArray, std::vector<int>& sortedIndices, int* memoryUsage = nullptr);

	/// <summary>
	/// <para/>Sort an array of uint16_t's using count sort/bucket sort. Indexes are stored in outputIndices.
	/// <para/>One can provide an array of input indices to influence the output indices. Useful for multi sorting.
	/// <para/>To get sorted results, iterate through outputIndices, using each element as an index into inputArray
	/// <para/>Uses more memory than radix sort. Memory usage is outputed to memoryUsage pointer.
	/// </summary>
	/// <param name="inputArray"> - unsorted input array</param>
	/// <param name="inputIndices"> - array of indices to use during sort, useful for multisort</param>
	/// <param name="sortedIndices"> - array of indices to unsorted input array, used to produce sorted array</param>
	/// <param name="memoryUsage"> - how much memory is created and used during the sort</param>
	void sortFast(std::vector<uint16_t>& inputArray, std::vector<int>& inputIndices, std::vector<int>& sortedIndices, int* memoryUsage = nullptr);

	/// <summary>
	/// <para/>Sort an array of uint16_t's using radix sort. Indexes are stored in outputIndices.
	/// <para/>To get sorted results, iterate through outputIndices, using each element as an index into inputArray
	/// <para/>Memory usage is outputed to memoryUsage pointer.
	/// </summary>
	/// <param name="inputArray"> - unsorted input array</param>
	/// <param name="sortedIndices"> - array of indices to unsorted input array, used to produce sorted array</param>
	/// <param name="memoryUsage"> - how much memory is created and used during the sort</param>
	void sortRadix(std::vector<uint16_t>& inputArray, std::vector<int>& sortedIndices, int* memoryUsage = nullptr);

	/// <summary>
	/// <para/>Sort an array of uint16_t's using radix sort. Indexes are stored in outputIndices.
	/// <para/>One can provide an array of input indices to influence the output indices. Useful for multi sorting.
	/// <para/>To get sorted results, iterate through outputIndices, using each element as an index into inputArray 
	/// <para/>Memory usage is outputed to memoryUsage pointer.
	/// </summary>
	/// <param name="inputArray"> - unsorted input array</param>
	/// <param name="inputIndices"> - array of indices to use during sort, useful for multisort</param>
	/// <param name="sortedIndices"> - array of indices to unsorted input array, used to produce sorted array</param>
	/// <param name="memoryUsage"> - how much memory is created and used during the sort</param>
	void sortRadix(std::vector<uint16_t>& inputArray, std::vector<int>& inputIndices, std::vector<int>& sortedIndices, int* memoryUsage = nullptr);
};

#endif
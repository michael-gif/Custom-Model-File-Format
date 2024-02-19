#ifndef SRC_STRIPERNEW_RADIXSORTER_H_
#define SRC_STRIPERNEW_RADIXSORTER_H_

class RadixSorter {
private:
	uint32_t* histogram; // Counters for each byte
	uint32_t* offset; // Offsets (nearly a cumulative distribution function)
	uint32_t currentSize; // Current size of the indices list
	uint32_t* indices; // Two lists, swapped each pass
	uint32_t* indices2;

public:
	RadixSorter();
	~RadixSorter();

	RadixSorter& sort(uint32_t* input, uint32_t nb, bool signedvalues=true);
	RadixSorter& sort(float* input, uint32_t nb);
	uint32_t* getIndices() { return indices; }
	RadixSorter& resetIndices();
	uint32_t getUsedRam();
};

#endif

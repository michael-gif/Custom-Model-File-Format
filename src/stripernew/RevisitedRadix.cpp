#include "Stdafx.h"
#include <iostream>

RadixSorter::RadixSorter() {
	// Initialize
	indices = null;
	indices2 = null;
	currentSize = 0;

	// Allocate input-independent ram
	histogram = new uint32_t[256 * 4];
	offset = new uint32_t[256];

	// Initialize indices
	resetIndices();
}

RadixSorter::~RadixSorter() {
	// Release everything
	RELEASEARRAY(offset);
	RELEASEARRAY(histogram);
	RELEASEARRAY(indices2);
	RELEASEARRAY(indices);
}

RadixSorter& RadixSorter::sort(uint32_t* input, uint32_t nb, bool signedvalues)
{
	// Resize lists if needed
	if(nb > currentSize) {
		// Free previously used ram
		RELEASEARRAY(this->indices2);
		RELEASEARRAY(this->indices);

		// Get some fresh one
		this->indices = new uint32_t[nb];
		this->indices2 = new uint32_t[nb];
		currentSize = nb;

		// Initialize indices so that the input buffer is read in sequential order
		resetIndices();
	}

	// Clear counters
	ZeroMemory(histogram, 256 * 4 * sizeof(uint32_t));

	// Create histograms (counters). Counters for all passes are created in one run.
	// Pros:	read input buffer once instead of four times
	// Cons:	histogram is 4Kb instead of 1Kb
	// We must take care of signed/unsigned values for temporal coherence.... I just
	// have 2 code paths even if just a single opcode changes. Self-modifying code, someone?

	// Temporal coherence
	bool alreadySorted = true;						// Optimism...
	uint32_t* indices = this->indices;
	// Prepare to count
	uint8_t* p = (uint8_t*)input;
	uint8_t* pe = &p[nb * 4];
	uint32_t* h0 = &histogram[0]; // Histogram for first pass (LSB)
	uint32_t* h1 = &histogram[256]; // Histogram for second pass
	uint32_t* h2 = &histogram[512]; // Histogram for third pass
	uint32_t* h3 = &histogram[768]; // Histogram for last pass (MSB)
	if(!signedvalues) {
		// Temporal coherence
		uint32_t prevVal = input[this->indices[0]];

		while(p != pe) {
			// Temporal coherence
			uint32_t val = input[*indices++]; // Read input buffer in previous sorted order
			if(val < prevVal) alreadySorted = false; // Check whether already sorted or not
			prevVal = val; // Update for next iteration

			// Create histograms
			h0[*p++]++;	h1[*p++]++;	h2[*p++]++;	h3[*p++]++;
		}
	} else {
		// Temporal coherence
		int prevVal = (int)input[this->indices[0]];

		while(p != pe) {
			// Temporal coherence
			int val = (int)input[*indices++]; // Read input buffer in previous sorted order
			if(val < prevVal)	alreadySorted = false; // Check whether already sorted or not
			prevVal = val; // Update for next iteration

			// Create histograms
			h0[*p++]++;	h1[*p++]++;	h2[*p++]++;	h3[*p++]++;
		}
	}

	// If all input values are already sorted, we just have to return and leave the previous list unchanged.
	// That way the routine may take advantage of temporal coherence, for example when used to sort transparent faces.
	if(alreadySorted) return *this;

	// Compute #negative values involved if needed
	uint32_t negativeValueCount = 0;
	if(signedvalues) {
		// An efficient way to compute the number of negatives values we'll have to deal with is simply to sum the 128
		// last values of the last histogram. Last histogram because that's the one for the Most Significant Byte,
		// responsible for the sign. 128 last values because the 128 first ones are related to positive numbers.
		uint32_t* h3 = &histogram[768];
		for(int i = 128; i < 256; i++) negativeValueCount += h3[i];	// 768 for last histogram, 128 for negative part
	}

	// Radix sort, j is the pass number (0=LSB, 3=MSB)
	for(int j = 0; j < 4; j++) {
		// Shortcut to current counters
		uint32_t* curCount = &histogram[j << 8];

		// Reset flag. The sorting pass is supposed to be performed. (default)
		bool performPass = true;

		// Check pass validity [some cycles are lost there in the generic case, but that's ok, just a little loop]
		for(int i = 0; i < 256; i++) {
			// If all values have the same byte, sorting is useless. It may happen when sorting bytes or words instead of dwords.
			// This routine actually sorts words faster than dwords, and bytes faster than words. Standard running time (O(4*n))is
			// reduced to O(2*n) for words and O(n) for bytes. Running time for floats depends on actual values...
			if(curCount[i] == nb) {
				performPass = false;
				break;
			}
			// If at least one count is not null, we suppose the pass must be done. Hence, this test takes very few CPU time in the generic case.
			if(curCount[i])	break;
		}

		// Sometimes the fourth (negative) pass is skipped because all numbers are negative and the MSB is 0xFF (for example). This is
		// not a problem, numbers are correctly sorted anyway.
		if(performPass) {
			// Should we care about negative values?
			if(j != 3 || !signedvalues) {
				// Here we deal with positive values only
				// Create offsets
				offset[0] = 0;
				for(int i = 1; i < 256; i++) offset[i] = offset[i - 1] + curCount[i - 1];
			} else {
				// This is a special case to correctly handle negative integers. They're sorted in the right order but at the wrong place.
				// Create biased offsets, in order for negative numbers to be sorted as well
				offset[0] = negativeValueCount;												// First positive number takes place after the negative ones
				for(int i = 1; i < 128; i++) offset[i] = offset[i - 1] + curCount[i - 1];	// 1 to 128 for positive numbers

				// Fixing the wrong place for negative values
				offset[128] = 0;
				for(int i = 129; i < 256; i++) offset[i] = offset[i - 1] + curCount[i - 1];
			}

			// Perform Radix Sort
			uint8_t* inputBytes = (uint8_t*)input;
			uint32_t* indices = this->indices;
			uint32_t* indicesEnd = &this->indices[nb];
			inputBytes += j;
			while(indices != indicesEnd) {
				uint32_t id = *indices++;
				this->indices2[offset[inputBytes[id << 2]]++] = id;
			}

			// Swap pointers for next pass
			uint32_t* tmp = this->indices;
			this->indices = this->indices2;
			this->indices2 = tmp;
		}
	}
	return *this;
}

RadixSorter& RadixSorter::sort(float* input2, uint32_t nb)
{
	uint32_t* input = (uint32_t*)input2;

	// Resize lists if needed
	if(nb > currentSize) {
		// Free previously used ram
		RELEASEARRAY(this->indices2);
		RELEASEARRAY(this->indices);

		// Get some fresh one
		this->indices = new uint32_t[nb];
		this->indices2 = new uint32_t[nb];
		currentSize = nb;

		// Initialize indices so that the input buffer is read in sequential order
		resetIndices();
	}

	// Clear counters
	ZeroMemory(histogram, 256 * 4 * sizeof(uint32_t));

	// Create histograms (counters). Counters for all passes are created in one run.
	// Pros:	read input buffer once instead of four times
	// Cons:	histogram is 4Kb instead of 1Kb
	// Floating-point values are always supposed to be signed values, so there's only one code path there.
	// Please note the floating point comparison needed for temporal coherence! Although the resulting asm code
	// is dreadful, this is surprisingly not such a performance hit - well, I suppose that's a big one on first
	// generation Pentiums....We can't make comparison on integer representations because, as Chris said, it just
	// wouldn't work with mixed positive/negative values....
	{
		// 3 lines for temporal coherence support
		float prevVal = input2[this->indices[0]];
		bool alreadySorted = true; // Optimism...
		uint32_t* indices = this->indices;

		// Prepare to count
		uint8_t* p = (uint8_t*)input;
		uint8_t* pe = &p[nb * 4];
		uint32_t* h0 = &histogram[0]; // Histogram for first pass (LSB)
		uint32_t* h1 = &histogram[256]; // Histogram for second pass
		uint32_t* h2 = &histogram[512]; // Histogram for third pass
		uint32_t* h3 = &histogram[768]; // Histogram for last pass (MSB)
		while(p != pe) {
			// Temporal coherence
			float val = input2[*indices++]; // Read input buffer in previous sorted order
			if(val < prevVal)	alreadySorted = false; // Check whether already sorted or not
			prevVal = val; // Update for next iteration

			// Create histograms
			h0[*p++]++;	h1[*p++]++;	h2[*p++]++;	h3[*p++]++;
		}

		// If all input values are already sorted, we just have to return and leave the previous list unchanged.
		// That way the routine may take advantage of temporal coherence, for example when used to sort transparent faces.
		if(alreadySorted) return *this;
	}

	// Compute #negative values involved if needed
	uint32_t negativeValueCount = 0;
	// An efficient way to compute the number of negatives values we'll have to deal with is simply to sum the 128
	// last values of the last histogram. Last histogram because that's the one for the Most Significant Byte,
	// responsible for the sign. 128 last values because the 128 first ones are related to positive numbers.
	uint32_t* h3 = &histogram[768];
	for(int i = 128; i < 256; i++) negativeValueCount += h3[i];	// 768 for last histogram, 128 for negative part

	// Radix sort, j is the pass number (0=LSB, 3=MSB)
	for(int j = 0; j < 4; j++) {
		// Shortcut to current counters
		uint32_t* curCount = &histogram[j << 8];

		// Reset flag. The sorting pass is supposed to be performed. (default)
		bool performPass = true;

		// Check pass validity [some cycles are lost there in the generic case, but that's ok, just a little loop]
		for(int i = 0; i < 256; i++) {
			// If all values have the same byte, sorting is useless. It may happen when sorting bytes or words instead of dwords.
			// This routine actually sorts words faster than dwords, and bytes faster than words. Standard running time (O(4*n))is
			// reduced to O(2*n) for words and O(n) for bytes. Running time for floats depends on actual values...
			if(curCount[i] == nb) {
				performPass = false;
				break;
			}
			// If at least one count is not null, we suppose the pass must be done. Hence, this test takes very few CPU time in the generic case.
			if(curCount[i])	break;
		}

		if(performPass) {
			// Should we care about negative values?
			if(j != 3) {
				// Here we deal with positive values only

				// Create offsets
				offset[0] = 0;
				for(int i = 1; i < 256; i++)
					offset[i] = offset[i-1] + curCount[i-1];

				// Perform Radix Sort
				uint8_t* inputBytes = (uint8_t*)input;
				uint32_t* indices = this->indices;
				uint32_t* indicesEnd = &this->indices[nb];
				inputBytes += j;
				while(indices != indicesEnd) {
					uint32_t id = *indices++;
					this->indices2[offset[inputBytes[id << 2]]++] = id;
				}
			} else {
				// This is a special case to correctly handle negative values

				// Create biased offsets, in order for negative numbers to be sorted as well
				offset[0] = negativeValueCount;												// First positive number takes place after the negative ones
				for(uint32_t i = 1; i < 128; i++) offset[i] = offset[i - 1] + curCount[i - 1];	// 1 to 128 for positive numbers

				// We must reverse the sorting order for negative numbers!
				offset[255] = 0;
				for(int i = 0; i < 127; i++) offset[254 - i] = offset[255 - i] + curCount[255 - i];	// Fixing the wrong order for negative values
				for(int i = 128; i < 256; i++) offset[i] += curCount[i];							// Fixing the wrong place for negative values

				// Perform Radix Sort
				for(int i = 0; i < nb; i++) {
					uint32_t Radix = input[this->indices[i]]>>24;								// Radix byte, same as above. AND is useless here (uint32_t).
					// ### cmp to be killed. Not good. Later.
					if(Radix<128) this->indices2[offset[Radix]++] = this->indices[i];		// Number is positive, same as above
					else this->indices2[--offset[Radix]] = this->indices[i];		// Number is negative, flip the sorting order
				}
			}

			// Swap pointers for next pass
			uint32_t* tmp = this->indices;
			this->indices = this->indices2;
			this->indices2 = tmp;
		}
	}
	return *this;
}

RadixSorter& RadixSorter::resetIndices()
{
	for(uint32_t i = 0; i < currentSize; i++)
		this->indices[i] = i;
	return *this;
}

uint32_t RadixSorter::getUsedRam()
{
	uint32_t usedRam = 0;
	usedRam += 256 * 4 * sizeof(uint32_t); // Histograms
	usedRam += 256 * sizeof(uint32_t); // Offsets
	usedRam += 2 * currentSize * sizeof(uint32_t);	// 2 lists of indices
	return usedRam;
}
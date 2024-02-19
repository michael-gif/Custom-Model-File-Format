#ifndef SRC_STRIPERNEW_STRIPER_H_
#define SRC_STRIPERNEW_STRIPER_H_

struct StriperOptions {
	uint32_t faceCount; // #faces in source topo
	uint32_t* DFaces; // list of faces (dwords) or null
	uint16_t* WFaces; // list of faces (words) or null
	bool askforUINT16; // true => results are in words (else dwords)
	bool oneSided; // true => create one-sided strips
	bool SGIAlgorithm; // true => use the SGI algorithm, pick least connected faces first
	bool connectAllStrips; // true => create a single strip with void faces

	StriperOptions() {
		DFaces = null;
		WFaces = null;
		faceCount = 0;
		askforUINT16	= true;
		oneSided = true;
		SGIAlgorithm = true;
		connectAllStrips = false;
	}
};

struct StriperResult {
	uint32_t stripCount; // #strips created
	uint32_t* stripLengths; // Lengths of the strips (stripCount values)
	void* stripIndices; // The strips in words or dwords, depends on askforUINT16
	bool askforUINT16; // true => results are in words (else dwords)
};

class Striper {
private:
	Striper& freeUsedRam();
	uint32_t computeBestStrip(uint32_t face);
	uint32_t trackStrip(uint32_t face, uint32_t oldest, uint32_t middle, uint32_t* strip, uint32_t* faces, bool* tags);
	bool connectAllStrips(StriperResult& result);

	Adjacencies* adacencies; // Adjacency structures
	bool* tags; // face markers

	uint32_t stripCount; // The number of strips created for the mesh
	CustomArray* stripLengths; // Array to store strip lengths
	CustomArray* stripIndices; // Array to store strip indices

	uint32_t singleStripLength; // The length of the single strip
	CustomArray* singleStrip; // Array to store the single strip

	// Flags
	bool askforUINT16;
	bool oneSided;
	bool SGIAlgorithm;
	bool shouldConnectAllStrips;

public:
	Striper();
	~Striper();

	void setOptions(StriperOptions& options);
	bool init(StriperOptions& options);
	bool compute(StriperResult& result);
};

#endif

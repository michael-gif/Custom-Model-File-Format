#include "Stdafx.h"
#include <iostream>

Striper& Striper::freeUsedRam()
{
	RELEASE(singleStrip);
	RELEASE(stripIndices);
	RELEASE(stripLengths);
	RELEASEARRAY(tags);
	RELEASE(adacencies);
	return *this;
}

Striper::Striper() {
	freeUsedRam();
}

Striper::~Striper() {
	freeUsedRam();
}

void Striper::setOptions(StriperOptions& options) {
	askforUINT16 = options.askforUINT16;
	oneSided = options.oneSided;
	SGIAlgorithm = options.SGIAlgorithm;
	shouldConnectAllStrips = options.connectAllStrips;
}

/// <summary>
/// The time to create the adjacency list depends on the number of triangles in the mesh, so there is a very real chance it could fail while creating
/// the list, hence the need for an init method, rather than putting everything in the constructor for Striper.
/// </summary>
/// <param name="options"></param>
/// <returns></returns>
bool Striper::init(StriperOptions& options) {
	// Create adjacencies
	adacencies = new Adjacencies;
	if (!adacencies) return false;

	AdjacenciesCreate ac;
	ac.faceCount = options.faceCount;
	ac.DFaces = options.DFaces;
	ac.WFaces = options.WFaces;
	bool status = adacencies->init(ac);
	if (!status) { RELEASE(adacencies); return false; }
	status = adacencies->createDatabase();
	if (!status) { RELEASE(adacencies); return false; }

	setOptions(options);
	return true;
}

/// <summary>
/// Create triangle strips
/// </summary>
/// <param name="result"></param>
/// <returns></returns>
bool Striper::compute(StriperResult& result)
{
	// You must call init() first
	if(!adacencies)	return false;

	// Get some bytes
	stripLengths = new CustomArray;
	if(!stripLengths) return false;
	stripIndices = new CustomArray;
	if(!stripIndices) return false;
	tags = new bool[adacencies->faceCount];
	if(!tags) return false;
	uint32_t* connectivity	= new uint32_t[adacencies->faceCount];
	if(!connectivity) return false;

	// tags contains one bool/face. True=>the face has already been included in a strip
	ZeroMemory(tags, adacencies->faceCount*sizeof(bool));

	// Compute the number of connections for each face. This buffer is further recycled into
	// the insertion order, ie contains face indices in the order we should treat them
	ZeroMemory(connectivity, adacencies->faceCount*sizeof(uint32_t));
	if(SGIAlgorithm) {
		// Compute number of adjacent triangles for each face
		for(uint32_t i = 0; i < adacencies->faceCount; i++) {
			AdjTriangle* tri = &adacencies->faces[i];
			if(!IS_BOUNDARY(tri->adjacentTris[0])) connectivity[i]++;
			if(!IS_BOUNDARY(tri->adjacentTris[1])) connectivity[i]++;
			if(!IS_BOUNDARY(tri->adjacentTris[2])) connectivity[i]++;
		}

		// Sort by number of neighbors
		RadixSorter RS;
		uint32_t* Sorted = RS.sort(connectivity, adacencies->faceCount).getIndices();

		// The sorted indices become the order of insertion in the strips
		CopyMemory(connectivity, Sorted, adacencies->faceCount*sizeof(uint32_t));
	} else {
		// Default order
		for(uint32_t i = 0; i < adacencies->faceCount; i++) connectivity[i] = i;
	}

	stripCount = 0;	// #strips created
	uint32_t totalFaceCount	= 0; // #faces already transformed into strips
	uint32_t index = 0;	// Index of first face

	while(totalFaceCount!=adacencies->faceCount) {
		// Look for the first face [could be optimized]
		while(tags[connectivity[index]]) index++;
		uint32_t firstFace = connectivity[index];

		// Compute the three possible strips from this face and take the best
		totalFaceCount += computeBestStrip(firstFace);

		// Let's wrap
		stripCount++;
	}

	// Free now useless ram
	RELEASEARRAY(connectivity);
	RELEASEARRAY(tags);

	// Fill result structure and exit
	result.stripCount = this->stripCount;
	result.stripLengths = (uint32_t*)this->stripLengths->Collapse();
	result.stripIndices = this->stripIndices->Collapse();

	if(shouldConnectAllStrips) connectAllStrips(result);

	return true;
}

/// <summary>
/// Compute the three possible strips starting from a given face
/// </summary>
/// <param name="face"></param>
/// <returns></returns>
uint32_t Striper::computeBestStrip(uint32_t face)
{
	uint32_t* strip[3];		// Strips computed in the 3 possible directions
	uint32_t* faces[3];		// Faces involved in the 3 previous strips
	uint32_t length[3];		// Lengths of the 3 previous strips

	uint32_t firstLength[3];	// Lengths of the first parts of the strips are saved for culling

	// Starting references
	uint32_t firstVertices[3];
	uint32_t secondVertices[3];
	firstVertices[0] = adacencies->faces[face].vertexIndices[0];
	secondVertices[0] = adacencies->faces[face].vertexIndices[1];

	// Bugfix by Eric Malafeew!
	firstVertices[1] = adacencies->faces[face].vertexIndices[2];
	secondVertices[1] = adacencies->faces[face].vertexIndices[0];

	firstVertices[2] = adacencies->faces[face].vertexIndices[1];
	secondVertices[2] = adacencies->faces[face].vertexIndices[2];

	// Compute 3 strips
	for(int j = 0; j < 3; j++) {
		// Get some bytes for the strip and its faces
		strip[j] = new uint32_t[adacencies->faceCount + 2 + 1 + 2];	// max possible length is faceCount+2, 1 more if the first index gets replicated
		faces[j] = new uint32_t[adacencies->faceCount + 2];
		FillMemory(strip[j], (adacencies->faceCount + 2 + 1 + 2) * sizeof(uint32_t), 0xff);
		FillMemory(faces[j], (adacencies->faceCount + 2) * sizeof(uint32_t), 0xff);

		// Create a local copy of the tags
		bool* localTags	= new bool[adacencies->faceCount];
		CopyMemory(localTags, this->tags, adacencies->faceCount * sizeof(bool));

		// Track first part of the strip
		length[j] = trackStrip(face, firstVertices[j], secondVertices[j], &strip[j][0], &faces[j][0], localTags);

		// Save first length for culling
		firstLength[j] = length[j];
//		if(j==1)	firstLength[j]++;	// ...because the first face is written in reverse order for j==1

		// Reverse first part of the strip
		for(uint32_t i=0;i<length[j]/2;i++) {
			strip[j][i]	^= strip[j][length[j] - i - 1];
			strip[j][length[j] - i - 1] ^= strip[j][i];
			strip[j][i]	^= strip[j][length[j] - i - 1];
		}
		for(int i = 0; i < (length[j] - 2) / 2; i++) {
			faces[j][i]^= faces[j][length[j] - i - 3];
			faces[j][length[j] - i - 3]	^= faces[j][i];
			faces[j][i]^= faces[j][length[j] - i - 3];
		}

		// Track second part of the strip
		uint32_t newVertex0 = strip[j][length[j] - 3];
		uint32_t newVertex1 = strip[j][length[j] - 2];
		uint32_t extraLength = trackStrip(face, newVertex0, newVertex1, &strip[j][length[j] - 3], &faces[j][length[j] - 3], localTags);
		length[j] += extraLength - 3;

		// Free temp ram
		RELEASEARRAY(localTags);
	}

	// Look for the best strip among the three
	uint32_t longest = length[0];
	uint32_t best = 0;
	if(length[1] > longest)	{ longest = length[1]; best = 1; }
	if(length[2] > longest)	{ longest = length[2]; best = 2; }

	uint32_t faceCount = longest - 2;

	// Update global tags
	for(int j = 0; j < longest - 2; j++) tags[faces[best][j]] = true;

	// Flip strip if needed ("if the length of the first part of the strip is odd, the strip must be reversed")
	if(oneSided && firstLength[best]&1) {
		// Here the strip must be flipped. I hardcoded a special case for triangles and quads.
		if(longest == 3 || longest == 4) {
			// Flip isolated triangle or quad
			strip[best][1] ^= strip[best][2];
			strip[best][2] ^= strip[best][1];
			strip[best][1] ^= strip[best][2];
		} else {
			// "to reverse the strip, write it in reverse order"
			for(int j = 0; j < longest / 2; j++) {
				strip[best][j] ^= strip[best][longest - j - 1];
				strip[best][longest - j - 1] ^= strip[best][j];
				strip[best][j] ^= strip[best][longest - j - 1];
			}

			// "If the position of the original face in this new reversed strip is odd, you're done"
			uint32_t newPos = longest - firstLength[best];
			if(newPos &1) {
				// "Else replicate the first index"
				for(int j = 0; j < longest; j++) strip[best][longest - j] = strip[best][longest - j - 1];
				longest++;
			}
		}
	}

	// Copy best strip in the strip buffers
	for(int j = 0; j < longest; j++) {
		uint32_t vertexIndex = strip[best][j];
		if(askforUINT16) stripIndices->Store((uint16_t)vertexIndex); // Saves word reference
		else stripIndices->Store(vertexIndex); // Saves dword reference
	}
	stripLengths->Store(longest);

	// Free local ram
	for(int j = 0; j < 3; j++) {
		RELEASEARRAY(faces[j]);
		RELEASEARRAY(strip[j]);
	}

	// Returns #faces involved in the strip
	return faceCount;
}

/// <summary>
/// Extend a strip in a given direction starting from a given face
/// </summary>
/// <param name="face">starting face</param>
/// <param name="oldest">first two indices of the strip</param>
/// <param name="middle">started edge</param>
/// <param name="strip">buffer to store strip</param>
/// <param name="faces">buffer to store faces of strip</param>
/// <param name="tags">buffer to mark visited faces</param>
/// <returns>strip length</returns>
uint32_t Striper::trackStrip(uint32_t face, uint32_t oldest, uint32_t middle, uint32_t* strip, uint32_t* faces, bool* tags)
{
	uint32_t length = 2; // Initial length is 2 since we have 2 indices in input
	strip[0] = oldest; // First index of the strip
	strip[1] = middle; // Second index of the strip

	bool doTheStrip = true;
	while(doTheStrip) {
		uint32_t newest = adacencies->faces[face].oppositeVertex(oldest, middle); // Get the third index of a face given two of them
		strip[length++] = newest; // Extend the strip,...
		*faces++ = face; // ...keep track of the face,...
		tags[face] = true; // ...and mark it as "done".

		uint8_t curEdge = adacencies->faces[face].findEdge(middle, newest); // Get the edge ID...
		uint32_t link = adacencies->faces[face].adjacentTris[curEdge]; // ...and use it to catch the link to adjacent face.
		if (IS_BOUNDARY(link)) {
			doTheStrip = false; // If the face is no more connected, we're done...
		} else {
			face = MAKE_ADJ_TRI(link); // ...else the link gives us the new face index.
			if(tags[face])	doTheStrip = false; // Is the new face already done?
		}
		oldest = middle; // Shift the indices and wrap
		middle = newest;
	}
	return length;
}

/// <summary>
/// Links all strips into single strip
/// </summary>
/// <param name="result">result structure</param>
/// <returns>true if success</returns>
bool Striper::connectAllStrips(StriperResult& result)
{
	singleStrip = new CustomArray;
	if(!singleStrip) return false;

	singleStripLength = 0;
	uint16_t* wrefs	= askforUINT16 ? (uint16_t*)result.stripIndices : null;
	uint32_t* drefs	= askforUINT16 ? null : (uint32_t*)result.stripIndices;

	// Loop over strips and link them together
	for(uint32_t k = 0; k < result.stripCount; k++) {
		// Nothing to do for the first strip, we just copy it
		if(k) {
			// This is not the first strip, so we must copy two void vertices between the linked strips
			uint32_t lastRef = drefs ? drefs[-1] : (uint32_t)wrefs[-1];
			uint32_t firstRef = drefs ? drefs[0] : (uint32_t)wrefs[0];
			if(askforUINT16) singleStrip->Store((uint16_t)lastRef).Store((uint16_t)firstRef);
			else singleStrip->Store(lastRef).Store(firstRef);
			singleStripLength += 2;

			// Linking two strips may flip their culling. If the user asked for single-sided strips we must fix that
			if(oneSided) {
				// Culling has been inverted only if singleStripLength is odd
				if(singleStripLength&1) {
					// We can fix culling by replicating the first vertex once again...
					uint32_t secondRef = drefs ? drefs[1] : (uint32_t)wrefs[1];
					if(firstRef != secondRef) {
						if(askforUINT16) singleStrip->Store((uint16_t)firstRef);
						else singleStrip->Store(firstRef);
						singleStripLength++;
					} else {
						// ...but if flipped strip already begin with a replicated vertex, we just can skip it.
						result.stripLengths[k]--;
						if(wrefs) wrefs++;
						if(drefs) drefs++;
					}
				}
			}
		}

		// Copy strip
		for(uint32_t j = 0; j < result.stripLengths[k]; j++) {
			uint32_t vertexIndex = drefs ? drefs[j] : (uint32_t)wrefs[j];
			if(askforUINT16) singleStrip->Store((uint16_t)vertexIndex);
			else singleStrip->Store(vertexIndex);
		}
		if(wrefs) wrefs += result.stripLengths[k];
		if(drefs) drefs += result.stripLengths[k];
		singleStripLength += result.stripLengths[k];
	}

	// Update result structure
	result.stripCount = 1;
	result.stripIndices	= singleStrip->Collapse();
	result.stripLengths	= &singleStripLength;

	return true;
}

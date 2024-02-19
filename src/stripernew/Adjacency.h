#ifndef SRC_STRIPERNEW_ADJACENCY_H_
#define SRC_STRIPERNEW_ADJACENCY_H_

// Macros
#define MAKE_ADJ_TRI(x) (x&0x3fffffff)
#define GET_EDGE_NB(x) (x>>30)
#define IS_BOUNDARY(x) (x==0xffffffff)

struct AdjTriangle { // Should be derived from a triangle structure
	uint32_t vertexIndices[3]; // Vertex-references
	uint32_t adjacentTris[3]; // Links/References of adjacent triangles. The 2 most significant bits contains the counterpart edge in the adjacent triangle.
	uint8_t findEdge(uint32_t vertex0, uint32_t vertex1);
	uint32_t oppositeVertex(uint32_t vertex0, uint32_t vertex1);
};

struct AdjEdge {
	uint32_t vertex0; // Vertex reference
	uint32_t vertex1; // Vertex reference
	uint32_t faceIndex; // Owner face
};

struct AdjacenciesCreate {
	AdjacenciesCreate() {
		DFaces = NULL;
		WFaces = NULL;
		faceCount = 0;
	}
	uint32_t faceCount; // #faces in source topo
	uint32_t* DFaces; // list of faces (dwords) or null
	uint16_t* WFaces; // list of faces (words) or null
};

class Adjacencies {
private:
	uint32_t edgeCount;
	uint32_t currentFaceIndex;
	AdjEdge* edges;

	bool addTriangle(uint32_t vertex0, uint32_t vertex1, uint32_t vertex3);
	bool addEdge(uint32_t vertex0, uint32_t vertex1, uint32_t faceIndex);
	bool updateLink(uint32_t firstTri, uint32_t secondTri, uint32_t vertex0, uint32_t vertex1);
	uint8_t findEdge(AdjTriangle* tri, uint32_t vertex0, uint32_t vertex1);

public:
	Adjacencies() : edgeCount(0), currentFaceIndex(0), edges(NULL), faceCount(0), faces(NULL) {}
	~Adjacencies();

	uint32_t faceCount;
	AdjTriangle* faces;

	bool init(AdjacenciesCreate& create);
	bool createDatabase();
};

#endif
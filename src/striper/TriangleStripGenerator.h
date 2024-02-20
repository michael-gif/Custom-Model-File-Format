#ifndef SRC_STRIPER_TRIANGLESTRIPGENERATOR_H_
#define SRC_STRIPER_TRIANGLESTRIPGENERATOR_H_

#include <fbxsdk.h>
#include <model/MeshObject.h>

struct Edge {
	uint16_t v1;
	uint16_t v2;
};

struct AdjTriangle {
	Edge edges[3];
	int adjacentTris[3] = {-1, -1, -1};
	uint16_t vertices[3];

	void createEdges(int v1, int v2, int v3);
	int getEdgeIndex(uint16_t v1, uint16_t v2);
	int getOppositeVertex(uint16_t v1, uint16_t v2);
};

class Striper {
private:
	int getRemainingVertexIndex(int v1, int v2, int a, int b, int c);
	void createAdjacencies(std::vector<AdjTriangle>& adjacencies, int* vertices);
	void linkAdjacencies(std::vector<AdjTriangle>& adjacencies);
	void updateLink(AdjTriangle* triangles, int firstTri, int secondTri, uint16_t vertex0, uint16_t vertex1);
	void generateStrips(std::vector<AdjTriangle>& adjacencies, int numAdjacencies, std::vector<std::vector<uint16_t>>& strips);
public:
	void striper(FbxMesh* inMesh, MeshObject* outMesh);
	void striper2(FbxMesh* inMesh, MeshObject* outMesh);
};

#endif
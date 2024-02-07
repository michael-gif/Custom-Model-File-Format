#include <striper/TriangleStripGenerator.h>
#include <model/MeshObject.h>
#include <util/Timer.hpp>
#include <string>
#include <numeric>
#include <chrono>
#include <fbxsdk.h>
#include <algorithm>

int ifSharesEdgeGetRemainingVertexIndex(uint32_t currentEdge, uint32_t edgeA, uint32_t edgeB, uint32_t edgeC, int v1, int v2, int v3) {
	if (currentEdge == edgeA) return v3;
	if (currentEdge == edgeB) return v1;
	if (currentEdge == edgeC) return v2;
	return -1;
}

void striper(FbxMesh* inMesh, MeshObject* mesh) {

	std::vector<int> currentStrip;
	std::vector<int> triangleIndices(mesh->vertexIndices.size() / 3);
	std::iota(triangleIndices.begin(), triangleIndices.end(), 0);

	auto start = Timer::begin();
	while (true) {
		int firstTriangleIndex = triangleIndices[0] * 3;
		int firstStartIndex = inMesh->GetPolygonVertexIndex(triangleIndices[0]);
		int* firstTriangleVertexPointer = &mesh->vertexIndices[firstTriangleIndex];
		currentStrip.emplace_back(*firstTriangleVertexPointer);
		currentStrip.emplace_back(*(firstTriangleVertexPointer + 1));
		currentStrip.emplace_back(*(firstTriangleVertexPointer + 2));
		uint32_t* firstTriangleEdgePointer = &mesh->edges[firstTriangleIndex];
		uint32_t backEdge = *(firstTriangleEdgePointer); // edge 1
		uint32_t frontEdge = *(firstTriangleEdgePointer + 1); // edge 2

		triangleIndices.erase(triangleIndices.begin());
		int remainingTriangles = triangleIndices.size();
		while (true) {
			bool foundTriangles = false;
			for (int i = 0; i < remainingTriangles; ++i) {
				int triangleIndex = triangleIndices[i] * 3;
				uint32_t* edgePointer = &mesh->edges[triangleIndex];
				uint32_t edge1 = *edgePointer;
				uint32_t edge2 = *(edgePointer + 1);
				uint32_t edge3 = *(edgePointer + 2);
				int* vertexPointer = &mesh->vertexIndices[triangleIndex];
				int v1 = *vertexPointer;
				int v2 = *(vertexPointer + 1);
				int v3 = *(vertexPointer + 2);
				uint32_t remainingIndex = ifSharesEdgeGetRemainingVertexIndex(frontEdge, edge1, edge2, edge3, v1, v2, v3);
				if (remainingIndex != -1) {
					foundTriangles = true;
					currentStrip.emplace_back(remainingIndex);
					int secondLastIndex = currentStrip[currentStrip.size() - 2];
					if (secondLastIndex < remainingIndex)
						frontEdge = (secondLastIndex << 16) | remainingIndex;
					else
						frontEdge = (remainingIndex << 16) | secondLastIndex;

					triangleIndices.erase(triangleIndices.begin() + i);
					i--;
					remainingTriangles--;
					continue;
				}
				uint32_t remainingIndex2 = ifSharesEdgeGetRemainingVertexIndex(backEdge, edge1, edge2, edge3, v1, v2, v3);
				if (remainingIndex2 != -1) {
					foundTriangles = true;
					currentStrip.insert(currentStrip.begin(), remainingIndex2);
					int firstIndex = currentStrip[0];
					int secondIndex = currentStrip[1];
					if (firstIndex < secondIndex)
						backEdge = (firstIndex << 16) | secondIndex;
					else
						backEdge = (secondIndex << 16) | firstIndex;

					triangleIndices.erase(triangleIndices.begin() + i);
					i--;
					remainingTriangles--;
					continue;
				}
			}
			if (!foundTriangles) break;
		}
		mesh->triangleStrips.emplace_back(currentStrip);
		currentStrip.clear();
		if (triangleIndices.empty()) break;
	}

	Timer::end(start, "Generated (" + std::to_string(mesh->triangleStrips.size()) + ") strips: ");
}
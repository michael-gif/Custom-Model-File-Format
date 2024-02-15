#include <string>
#include <numeric>
#include <fbxsdk.h>
#include <model/MeshObject.h>
#include <striper/TriangleStripGenerator.h>
#include <util/Timer.hpp>
#include <util/ProgressBar.hpp>

int getRemainingVertexIndex(int v1, int v2, int a, int b, int c)
{
	if (v1 == a) {
		if (v2 == b) return c;
		if (v2 == c) return b;
	}
	else if (v1 == b) {
		if (v2 == a) return c;
		if (v2 == c) return a;
	}
	else if (v1 == c) {
		if (v2 == b) return a;
		if (v2 == a) return b;
	}
	return -1;
}

void striper(FbxMesh* inMesh, MeshObject* outMesh)
{
	int polygonCount;
#if _DEBUG
	auto readStart = Timer::begin();
	polygonCount = inMesh->GetPolygonCount();
	Timer::end(readStart, "Found (" + std::to_string(polygonCount) + ") triangles: ");
#endif
	auto start = Timer::begin();
	polygonCount = inMesh->GetPolygonCount();
	std::vector<int> triangles(polygonCount);
	std::vector<uint16_t> currentStrip(3);

	// array indexes are slow. use pointers instead
	int* vertices = inMesh->GetPolygonVertices();
	int* trianglesPtr = triangles.data();
	uint16_t* stripPtr = currentStrip.data();

	int stripNumber = 0;
	int sizeondisk = 0;
	int numTriangles = (int)triangles.size();
	std::iota(triangles.begin(), triangles.end(), 0);

	ProgressBar progressBar(polygonCount);
	progressBar.start();


	while (true) {
		int firstTriangleIndex = trianglesPtr[0] * 3;
		int frontX = vertices[firstTriangleIndex];
		int frontY = vertices[firstTriangleIndex + 1];
		int frontZ = vertices[firstTriangleIndex + 2];
		int backX = frontX;
		int backY = frontY;
		stripPtr[0] = frontX;
		stripPtr[1] = frontY;
		stripPtr[2] = frontZ;
		triangles.erase(triangles.begin());
		numTriangles--;
		while (true) {
			int adjacentTris = 0;
			for (int i = 0; i < numTriangles; ++i) {
				int triangleIndex = trianglesPtr[i] * 3;
				int v1 = vertices[triangleIndex];
				int v2 = vertices[triangleIndex + 1];
				int v3 = vertices[triangleIndex + 2];

				int adjacentFront = getRemainingVertexIndex(frontY, frontZ, v1, v2, v3);
				if (adjacentFront != -1) {
					++adjacentTris;
					frontY = frontZ;
					frontZ = adjacentFront;
					currentStrip.emplace_back((uint16_t)adjacentFront);

					// erase triangle index from vector and update loop
					triangles.erase(triangles.begin() + i);
					--i; --numTriangles;
					continue;
				}
				int adjacentBehind = getRemainingVertexIndex(backX, backY, v1, v2, v3);
				if (adjacentBehind != -1) {
					++adjacentTris;
					backY = backX;
					backX = adjacentBehind;
					currentStrip.insert(currentStrip.begin(), (uint16_t)adjacentBehind);

					// erase triangle index from vector and update loop
					triangles.erase(triangles.begin() + i);
					--i; --numTriangles;
					continue;
				}
			}
			if (!adjacentTris) break; // if no adjacent triangles were found, the strip has ended.
		}
#if _DEBUG
		sizeondisk += 2 + ((int)currentStrip.size() * 2); // statistics
#endif
		outMesh->triangleStrips.emplace_back(currentStrip);
		currentStrip.clear();
		currentStrip.resize(3);
		progressBar.updateProgress(polygonCount - triangles.size());

		if (triangles.empty()) break; // no more triangles mean we have created all the strips needed.
	}
	std::cout << "0" << std::endl;
	Timer::end(start, "[MODELMAKER] Generated (" + std::to_string(outMesh->triangleStrips.size()) + ") triangle strips: ");
#if _DEBUG
	std::cout << "Size on disk: " << sizeondisk << " bytes\n";
#endif
}

int getRemainingIndex(uint32_t edge, uint32_t edgeA, uint32_t edgeB, uint32_t edgeC, uint64_t vertices)
{
	if (edge == edgeA) return (vertices >> 48);
	if (edge == edgeB) return (vertices >> 16) & 65535;
	if (edge == edgeC) return (vertices >> 32) & 65535;
	return -1;
}

void striper2(FbxMesh* inMesh, MeshObject* outMesh)
{
	auto start = Timer::begin();
	int polygonCount = inMesh->GetPolygonCount();
	std::vector<uint16_t> currentStrip;
	std::vector<uint16_t> triangleIndices(polygonCount);
	std::vector<uint16_t> vertexIndices(static_cast<size_t>(polygonCount) * 4);

	// array indexes are slow. use pointer access instead
	int* vertexPointer = inMesh->GetPolygonVertices();
	uint16_t* triangleIndicesPtr = triangleIndices.data();
	uint32_t* edgePointer = outMesh->edges.data();
	uint16_t* paddedVertexIndicesPtr = vertexIndices.data();
	uint64_t* vertexIndicesPtr = (uint64_t*)paddedVertexIndicesPtr;

	std::iota(triangleIndices.begin(), triangleIndices.end(), 0);
	// insert a 0 before every 3 vertex indices. this way they indices can be read as a uint64_t then masked later, reducing array accesses
	for (int i = 0; i < polygonCount; ++i) {
		int paddedStartIndex = i * 4;
		int vertexStartIndex = i * 3;
		paddedVertexIndicesPtr[paddedStartIndex + 1] = vertexPointer[vertexStartIndex];
		paddedVertexIndicesPtr[paddedStartIndex + 2] = vertexPointer[vertexStartIndex + 1];
		paddedVertexIndicesPtr[paddedStartIndex + 3] = vertexPointer[vertexStartIndex + 2];
	}
	int sizeondisk = 0;
	int remainingTriangles = polygonCount;

	// for progress bar
	ProgressBar progressBar(polygonCount);
	progressBar.start();

	while (true) {
		int firstTriangleIndex = triangleIndicesPtr[0] * 3;
		currentStrip.emplace_back(vertexPointer[firstTriangleIndex]);
		currentStrip.emplace_back(vertexPointer[firstTriangleIndex + 1]);
		currentStrip.emplace_back(vertexPointer[firstTriangleIndex + 2]);
		uint32_t backEdge = edgePointer[firstTriangleIndex]; // edge 1
		uint32_t frontEdge = edgePointer[firstTriangleIndex + 1]; // edge 2

		triangleIndices.erase(triangleIndices.begin());
		remainingTriangles--;
		while (true) {
			bool foundTriangles = false;
			for (int i = 0; i < remainingTriangles; ++i) {
				int triangleIndex = triangleIndicesPtr[i];
				int triangleVerticesIndex = triangleIndex * 3;
				uint32_t edge1 = edgePointer[triangleVerticesIndex];
				uint32_t edge2 = edgePointer[triangleVerticesIndex + 1];
				uint32_t edge3 = edgePointer[triangleVerticesIndex + 2];
				uint64_t vertices = vertexIndicesPtr[triangleIndex];

				int remainingIndex = getRemainingIndex(frontEdge, edge1, edge2, edge3, vertices);
				if (remainingIndex != -1) {
					foundTriangles = true;
					int secondLastIndex = currentStrip.back();
					currentStrip.emplace_back((uint16_t)remainingIndex);
					if (secondLastIndex < remainingIndex)
						frontEdge = (secondLastIndex << 16) | remainingIndex;
					else
						frontEdge = (remainingIndex << 16) | secondLastIndex;

					triangleIndices.erase(triangleIndices.begin() + i);
					i--;
					remainingTriangles--;
					continue;
				}
				int remainingIndex2 = getRemainingIndex(backEdge, edge1, edge2, edge3, vertices);
				if (remainingIndex2 != -1) {
					foundTriangles = true;
					currentStrip.insert(currentStrip.begin(), (uint16_t)remainingIndex2);
					uint16_t* currentStripPtr = currentStrip.data();
					uint16_t firstIndex = currentStripPtr[0];
					uint16_t secondIndex = currentStripPtr[1];
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
#if _DEBUG
		sizeondisk += 2 + ((int)currentStrip.size() * 2); // statistics
#endif
		outMesh->triangleStrips.emplace_back(currentStrip);
		currentStrip.clear();
		progressBar.updateProgress(polygonCount - triangleIndices.size());

		if (triangleIndices.empty()) break;
	}
	std::cout << "0" << std::endl;
	Timer::end(start, "[MODELMAKER] Generated (" + std::to_string(outMesh->triangleStrips.size()) + ") strips: ");
#if _DEBUG
	std::cout << "Size on disk: " << sizeondisk << " bytes\n";
#endif
}
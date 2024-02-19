#include <string>
#include <numeric>
#include <fbxsdk.h>
#include <iomanip>
#include <model/MeshObject.h>
#include <striper/TriangleStripGenerator.h>
#include <util/Timer.hpp>
#include <util/ProgressBar.hpp>
#include <striper/RadixSorter.h>

int Striper::getRemainingVertexIndex(int v1, int v2, int a, int b, int c)
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

void Striper::striper(FbxMesh* inMesh, MeshObject* outMesh)
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

int Striper::getRemainingIndex(uint32_t edge, uint32_t edgeA, uint32_t edgeB, uint32_t edgeC, uint64_t vertices)
{
	if (edge == edgeA) return (vertices >> 48);
	if (edge == edgeB) return (vertices >> 16) & 65535;
	if (edge == edgeC) return (vertices >> 32) & 65535;
	return -1;
}

void Striper::striper2(FbxMesh* inMesh, MeshObject* outMesh)
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

void AdjTriangle::createEdges(int v1, int v2, int v3)
{
	if (v1 < v2) {
		edges[0].v1 = v1;
		edges[0].v2 = v2;
	}
	else {
		edges[0].v1 = v2;
		edges[0].v2 = v1;
	}
	if (v2 < v3) {
		edges[1].v1 = v2;
		edges[1].v2 = v3;
	}
	else {
		edges[1].v1 = v3;
		edges[1].v2 = v2;
	}
	if (v3 < v1) {
		edges[2].v1 = v3;
		edges[2].v2 = v1;
	}
	else {
		edges[2].v1 = v1;
		edges[2].v2 = v3;
	}
}

int AdjTriangle::getEdgeIndex(uint16_t v1, uint16_t v2)
{
	Edge* e0 = &edges[0];
	if (v1 == e0->v1 && v2 == e0->v2) return 0;
	Edge* e1 = &edges[1];
	if (v1 == e1->v1 && v2 == e1->v2) return 1;
	Edge* e2 = &edges[2];
	if (v1 == e2->v1 && v2 == e2->v2) return 2;
}

int AdjTriangle::getOppositeVertex(int v1, int v2)
{
	if (v1 == vertices[0]) {
		if (v2 == vertices[1]) return vertices[2];
		if (v2 == vertices[2]) return vertices[1];
	} else if (v1 == vertices[1]) {
		if (v2 == vertices[2]) return vertices[0];
		if (v2 == vertices[0]) return vertices[2];
	} else if (v1 == vertices[2]) {
		if (v2 == vertices[0]) return vertices[1];
		if (v2 == vertices[1]) return vertices[0];
	}
}

/// <summary>
/// For each triangle, create an AdjTriangle struct with 3 edges
/// </summary>
/// <param name="adjacencies">Empty adjacency vector to populate</param>
/// <param name="vertices">Vertex array to create triangles from</param>
void Striper::createAdjacencies(std::vector<AdjTriangle>& adjacencies, int* vertices)
{
	auto start = Timer::begin();
	AdjTriangle* adjacencyPtr = adjacencies.data();
	for (int i = 0; i < adjacencies.size(); i++) {
		int vertexIndex = i * 3;
		int v1 = vertices[vertexIndex];
		int v2 = vertices[vertexIndex + 1];
		int v3 = vertices[vertexIndex + 2];
		adjacencyPtr[i].createEdges(v1, v2, v3);
	}
	Timer::end(start, "Created adjacencies: ");
}

/// <summary>
/// Create a link between two given triangles by updating their respective adjacency structures.
/// Each triangle has an array of adjacent triangles, and an array of edges. Each index of the adjacent triangles array maps to
/// the indexes of the edges array. If a triangle has an adjacent triangle at index 0, then the adjacent triangle shares the edge
/// at index 0 of the edges array.
/// </summary>
/// <param name="triangles"></param>
/// <param name="firstTri"></param>
/// <param name="secondTri"></param>
/// <param name="vertex0"></param>
/// <param name="vertex1"></param>
/// <returns></returns>
void Striper::updateLink(AdjTriangle* triangles, int firstTri, int secondTri, uint16_t vertex0, uint16_t vertex1)
{
	AdjTriangle* tri0 = &triangles[firstTri];
	AdjTriangle* tri1 = &triangles[secondTri];
	int tri0EdgeIndex = tri0->getEdgeIndex(vertex0, vertex1);
	int tri1EdgeIndex = tri1->getEdgeIndex(vertex0, vertex1);
	tri0->adjacentTris[tri0EdgeIndex] = secondTri;
	tri1->adjacentTris[tri1EdgeIndex] = firstTri;
}

/// <summary>
/// Link the adjacency structures by creating a list of all edges in the mesh and sorting by the second vertex of each edge.
/// This creates a list of indexes, each index leading to an element in the unsorted list.
/// For example, if the unsorted edges are [4, 8, 2, 6, 3, 2], and the sorted indexes are [2, 5, 4, 0, 3, 1],
/// then indexing the unsorted edges using the sorted index array would produce [2, 2, 3, 4, 6, 8].
/// The sorted index array can then be used to obtain the corresponding first vertex for every second vertex, and the corresponding face for
/// every second vertex. By looping through the arrays you can quickly identify matching edges and create the corresponding links between triangles.
/// </summary>
/// <param name="adjacencies"></param>
/// <param name="triangleCount"></param>
void Striper::linkAdjacencies(std::vector<AdjTriangle>& adjacencies)
{
	auto start = Timer::begin();
	int edgeCount = adjacencies.size() * 3;
	std::vector<int> faceIndices(edgeCount); // every edge has an associated face
	std::vector<uint16_t> firstVertices(edgeCount);
	std::vector<uint16_t> secondVertices(edgeCount);

	AdjTriangle* adjacencyPtr = adjacencies.data();
	int* facesIndicesPtr = faceIndices.data();
	uint16_t* firstVerticesPtr = firstVertices.data();
	uint16_t* secondVerticesPtr = secondVertices.data();

	for (int i = 0; i < adjacencies.size(); i++) {
		int edgeIndex = i * 3;
		facesIndicesPtr[edgeIndex] = i;
		facesIndicesPtr[edgeIndex + 1] = i;
		facesIndicesPtr[edgeIndex + 2] = i;
		AdjTriangle* triangle = &adjacencyPtr[i];
		Edge* edges = triangle->edges;
		firstVerticesPtr[edgeIndex] = edges[0].v1;
		firstVerticesPtr[edgeIndex + 1] = edges[1].v1;
		firstVerticesPtr[edgeIndex + 2] = edges[2].v1;
		secondVerticesPtr[edgeIndex] = edges[0].v2;
		secondVerticesPtr[edgeIndex + 1] = edges[1].v2;
		secondVerticesPtr[edgeIndex + 2] = edges[2].v2;
	}
	
	//Sort the edges by first vertex then second vertex, ensuring that adjacent edges are next to each other
	//auto start3 = Timer::begin();
	RadixSorter sorter;
	std::vector<int> firstSortedIndices(edgeCount);
	std::vector<int> secondSortedIndices(edgeCount);
	sorter.sort(firstVertices, firstSortedIndices);
	sorter.sort(secondVertices, secondSortedIndices, firstSortedIndices);
	//Timer::end(start3, "Sorted indices: ");

	// Read the list in sorted order, creating links between adjacent triangles
	//auto start4 = Timer::begin();
	uint16_t lastVertex0 = firstVerticesPtr[secondSortedIndices[0]];
	uint16_t lastVertex1 = secondVerticesPtr[secondSortedIndices[0]];
	char count = 0;
	int tmpBuffer[3];

	for (int i = 0; i < edgeCount; i++) {
		int faceIndex = facesIndicesPtr[secondSortedIndices[i]];
		uint16_t vertex0 = firstVerticesPtr[secondSortedIndices[i]];
		uint16_t vertex1 = secondVerticesPtr[secondSortedIndices[i]];
		if (vertex0 == lastVertex0 && vertex1 == lastVertex1) {
			tmpBuffer[count] = faceIndex;
			count++;
			if (count == 3) return;
		} else {
			if (count == 2) updateLink(adjacencyPtr, tmpBuffer[0], tmpBuffer[1], lastVertex0, lastVertex1);
			// Reset for next edge
			tmpBuffer[0] = faceIndex;
			count = 1;
			lastVertex0 = vertex0;
			lastVertex1 = vertex1;
		}
	}
	if (count == 2) updateLink(adjacencyPtr, tmpBuffer[0], tmpBuffer[1], lastVertex0, lastVertex1);
	//Timer::end(start4, "Linking: ");

	//int memoryUsage = 0;
	//memoryUsage += faceIndices.size() * sizeof(int);
	//memoryUsage += firstVertices.size() * sizeof(uint16_t);
	//memoryUsage += secondVertices.size() * sizeof(uint16_t);
	//memoryUsage += firstSortedIndices.size() * sizeof(int);
	//memoryUsage += secondSortedIndices.size() * sizeof(int);
	//std::cout << "Memory usage: " << memoryUsage << " bytes\n";
	Timer::end(start, "Linked adjacencies: ");
}

void Striper::striper4(FbxMesh* inMesh, MeshObject* outMesh)
{
	auto start = Timer::begin();
	int triangleCount = inMesh->GetPolygonCount();
	std::cout << "Found (" << triangleCount << ") triangles" << std::endl;

	int* vertices = inMesh->GetPolygonVertices();
	std::vector<AdjTriangle> adjacencies(triangleCount);
	createAdjacencies(adjacencies, vertices);
	linkAdjacencies(adjacencies);

	//TODO walk through adjacencies to generate strips
}
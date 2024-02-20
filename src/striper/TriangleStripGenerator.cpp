#include <string>
#include <numeric>
#include <fbxsdk.h>
#include <iomanip>
#include <model/MeshObject.h>
#include <striper/TriangleStripGenerator.h>
#include <util/Timer.hpp>
#include <util/ProgressBar.hpp>
#include <striper/RadixSorter.h>

void AdjTriangle::createEdges(int v1, int v2, int v3)
{
	vertices[0] = v1;
	vertices[1] = v2;
	vertices[2] = v3;
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
	return -1;
}

int AdjTriangle::getOppositeVertex(uint16_t v1, uint16_t v2)
{
	if (v1 == vertices[0]) {
		if (v2 == vertices[1]) return vertices[2];
		if (v2 == vertices[2]) return vertices[1];
	}
	else if (v1 == vertices[1]) {
		if (v2 == vertices[2]) return vertices[0];
		if (v2 == vertices[0]) return vertices[2];
	}
	else if (v1 == vertices[2]) {
		if (v2 == vertices[0]) return vertices[1];
		if (v2 == vertices[1]) return vertices[0];
	}
	return -1;
}

/// <summary>
/// For each triangle, create an AdjTriangle struct with 3 edges
/// </summary>
/// <param name="adjacencies">Empty adjacency vector to populate</param>
/// <param name="vertices">Vertex array to create triangles from</param>
void Striper::createAdjacencies(std::vector<AdjTriangle>& adjacencies, int* vertices)
{
#if _DEBUG
	auto start = Timer::begin();
#endif

	AdjTriangle* adjacencyPtr = adjacencies.data();
	for (int i = 0; i < adjacencies.size(); i++) {
		int vertexIndex = i * 3;
		int v1 = vertices[vertexIndex];
		int v2 = vertices[vertexIndex + 1];
		int v3 = vertices[vertexIndex + 2];
		adjacencyPtr[i].createEdges(v1, v2, v3);
	}

#if _DEBUG
	Timer::end(start, "Created adjacencies: ");
#endif
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
#if _DEBUG
	auto start = Timer::begin();
#endif

	int edgeCount = (int)adjacencies.size() * 3;
	std::vector<int> faceIndices(edgeCount); // every edge has an associated face
	std::vector<uint16_t> firstVertices(edgeCount);
	std::vector<uint16_t> secondVertices(edgeCount);
	AdjTriangle* adjacencyPtr = adjacencies.data();

	for (int i = 0; i < adjacencies.size(); i++) {
		int edgeIndex = i * 3;
		faceIndices[edgeIndex] = i;
		faceIndices[edgeIndex + 1] = i;
		faceIndices[edgeIndex + 2] = i;
		AdjTriangle* triangle = &adjacencyPtr[i];
		Edge* edges = triangle->edges;
		firstVertices[edgeIndex] = edges[0].v1;
		firstVertices[edgeIndex + 1] = edges[1].v1;
		firstVertices[edgeIndex + 2] = edges[2].v1;
		secondVertices[edgeIndex] = edges[0].v2;
		secondVertices[edgeIndex + 1] = edges[1].v2;
		secondVertices[edgeIndex + 2] = edges[2].v2;
	}
	
	//Sort the edges by first vertex then second vertex, ensuring that adjacent edges are next to each other
	RadixSorter sorter;
	std::vector<int> firstSortedIndices(edgeCount);
	std::vector<int> secondSortedIndices(edgeCount);
	sorter.sortFast(firstVertices, firstSortedIndices);
	sorter.sortFast(secondVertices, firstSortedIndices, secondSortedIndices);

	// Read the list in sorted order, creating links between adjacent triangles
	uint16_t lastVertex0 = firstVertices[secondSortedIndices[0]];
	uint16_t lastVertex1 = secondVertices[secondSortedIndices[0]];
	char count = 0;
	int tmpBuffer[3];

	for (int i = 0; i < edgeCount; i++) {
		int faceIndex = faceIndices[secondSortedIndices[i]];
		uint16_t vertex0 = firstVertices[secondSortedIndices[i]];
		uint16_t vertex1 = secondVertices[secondSortedIndices[i]];
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

#if _DEBUG
	int memoryUsage = 0;
	memoryUsage += edgeCount * sizeof(int);
	memoryUsage += edgeCount * sizeof(uint16_t);
	memoryUsage += edgeCount * sizeof(uint16_t);
	memoryUsage += edgeCount * sizeof(int);
	memoryUsage += edgeCount * sizeof(int);
	std::cout << "Linking memory usage: " << memoryUsage << " bytes\n";
	Timer::end(start, "Linked adjacencies: ");
#endif
}

void Striper::generateStrips(std::vector<AdjTriangle>& adjacencies, int numAdjacencies, std::vector<std::vector<uint16_t>>& strips)
{
	auto start = Timer::begin();
	std::vector<int> indices(numAdjacencies);
	std::iota(indices.begin(), indices.end(), 0);
	int numTriangles = numAdjacencies;
	AdjTriangle* triangles = adjacencies.data();
	int* indicesPtr = indices.data();
	ProgressBar progressBar(numAdjacencies);
	progressBar.start();

	int lastTriangleIndex = 0;
	while (numTriangles > 0) {
		strips.emplace_back();
		std::vector<uint16_t>* strip = &strips.back();
		strip->resize(3);
		uint16_t* stripData = strip->data();
		int nextTriangleIndex = -1;
		for (int i = lastTriangleIndex; i < numAdjacencies; i++) {
			if (indicesPtr[i] != -1) {
				nextTriangleIndex = i;
				lastTriangleIndex = i + 1;
				break;
			}
		}
		AdjTriangle* firstTri = &triangles[indicesPtr[nextTriangleIndex]];
		AdjTriangle* currentFrontTri = firstTri;
		AdjTriangle* currentBackTri = firstTri;
		indicesPtr[nextTriangleIndex] = -1;
		numTriangles--;
		memcpy(stripData, firstTri->vertices, 3 * sizeof(uint16_t)); // move current triangle vertices into start of strip
		uint16_t firstVertex = stripData[0];
		uint16_t secondVertex = stripData[1];
		uint16_t thirdVertex = stripData[2];
		while (true) {
			int frontEdgeIndex;
			if (secondVertex < thirdVertex) frontEdgeIndex = currentFrontTri->getEdgeIndex(secondVertex, thirdVertex);
			else frontEdgeIndex = currentFrontTri->getEdgeIndex(thirdVertex, secondVertex);
			int adjacentTriIndex = currentFrontTri->adjacentTris[frontEdgeIndex];
			if (indicesPtr[adjacentTriIndex] != -1) {
				indicesPtr[adjacentTriIndex] = -1;
				numTriangles--;
				AdjTriangle* adjacentTri = &triangles[adjacentTriIndex];
				currentFrontTri = adjacentTri;
				int newVertex = adjacentTri->getOppositeVertex(secondVertex, thirdVertex);
				strip->emplace_back(newVertex);
				secondVertex = thirdVertex;
				thirdVertex = newVertex;
				continue;
			}
			else {
				stripData = strip->data();
				secondVertex = stripData[1];
				int backEdgeIndex;
				if (firstVertex < secondVertex) backEdgeIndex = currentBackTri->getEdgeIndex(firstVertex, secondVertex);
				else backEdgeIndex = currentBackTri->getEdgeIndex(secondVertex, firstVertex);
				int adjacentTriIndex = currentBackTri->adjacentTris[backEdgeIndex];
				if (indicesPtr[adjacentTriIndex] == -1) break;
				indicesPtr[adjacentTriIndex] = -1;
				numTriangles--;
				AdjTriangle* adjacentTri = &triangles[adjacentTriIndex];
				currentBackTri = adjacentTri;
				int newVertex = adjacentTri->getOppositeVertex(firstVertex, secondVertex);
				strip->insert(strip->begin(), newVertex);
				stripData = strip->data();
				secondVertex = stripData[strip->size() - 2];
				firstVertex = newVertex;
			}
		}
		progressBar.updateProgress(numAdjacencies - numTriangles);
	}
	Timer::end(start, "Found (" + std::to_string(strips.size()) + ") triangle strips: ");
}

void Striper::striper(FbxMesh* inMesh, MeshObject* outMesh)
{
#if _DEBUG
	auto start = Timer::begin();
	int triangleCount = inMesh->GetPolygonCount();
	std::cout << "Found (" << triangleCount << ") triangles" << std::endl;
#else
	int triangleCount = inMesh->GetPolygonCount();
#endif

	int* vertices = inMesh->GetPolygonVertices();
	std::vector<AdjTriangle> adjacencies(triangleCount);
	createAdjacencies(adjacencies, vertices);
	linkAdjacencies(adjacencies);
	generateStrips(adjacencies, triangleCount, outMesh->triangleStrips);

#if _DEBUG
	int memoryUsage = 0;
	memoryUsage += triangleCount * 36;
	std::cout << "Memory usage: " << memoryUsage << " bytes\n";
#endif
}
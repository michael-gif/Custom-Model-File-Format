#include <string>
#include <numeric>
#include <algorithm>
#include <immintrin.h>
#include <meshstriper/MeshStriper.h>
#include <meshstriper/Sorter.h>
#include <util/Timer.hpp>
#include <util/ProgressBar.hpp>

void AdjTriangle::createEdges(int* vertices, int vertexIndex)
{
	int v1 = vertices[vertexIndex];
	int v2 = vertices[vertexIndex + 1];
	int v3 = vertices[vertexIndex + 2];
	this->vertices[0] = v1;
	this->vertices[1] = v2;
	this->vertices[2] = v3;

	if (v1 < v2) {
		edges[0].v1 = v1;
		edges[0].v2 = v2;
		edges[0].edge = (v1 << 16) | v2;
	}
	else {
		edges[0].v1 = v2;
		edges[0].v2 = v1;
		edges[0].edge = (v2 << 16) | v1;
	}
	if (v2 < v3) {
		edges[1].v1 = v2;
		edges[1].v2 = v3;
		edges[1].edge = (v2 << 16) | v3;
	}
	else {
		edges[1].v1 = v3;
		edges[1].v2 = v2;
		edges[1].edge = (v3 << 16) | v2;
	}
	if (v3 < v1) {
		edges[2].v1 = v3;
		edges[2].v2 = v1;
		edges[2].edge = (v3 << 16) | v1;
	}
	else {
		edges[2].v1 = v1;
		edges[2].v2 = v3;
		edges[2].edge = (v1 << 16) | v3;
	}
}

int AdjTriangle::getEdgeIndex(uint16_t v1, uint16_t v2)
{
	Edge* e0 = &edges[0];
	if (v1 == e0->v1 && v2 == e0->v2) return 0;
	Edge* e1 = &edges[1];
	if (v1 == e1->v1 && v2 == e1->v2) return 1;
	return 2;
}

int AdjTriangle::getEdgeIndex(uint32_t edge)
{
	if (edges[0].edge == edge) return 0;
	if (edges[1].edge == edge) return 1;
	return 2;
}

uint16_t AdjTriangle::getOppositeVertex(uint16_t v1, uint16_t v2)
{
	if (v1 == vertices[0]) {
		if (v2 == vertices[1]) return vertices[2];
		if (v2 == vertices[2]) return vertices[1];
	}
	else if (v1 == vertices[1]) {
		if (v2 == vertices[2]) return vertices[0];
		if (v2 == vertices[0]) return vertices[2];
	}
	else {
		if (v2 == vertices[0]) return vertices[1];
		return vertices[0];
	}
}

void MeshStriper::createTriangleStructures(std::vector<AdjTriangle>& adjacencies, int* vertices)
{
#if _DEBUG
	auto start = Timer::begin();
#endif

	AdjTriangle* adjacencyPtr = adjacencies.data();
	int numAdjacencies = adjacencies.size();
	int remaining = numAdjacencies % 4;
	for (int i = 0; i < numAdjacencies; i += 4) {
		adjacencyPtr[i].createEdges(vertices, i * 3);
		adjacencyPtr[i+1].createEdges(vertices, (i+1) * 3);
		adjacencyPtr[i+2].createEdges(vertices, (i+2) * 3);
		adjacencyPtr[i+3].createEdges(vertices, (i+3) * 3);
	}
	for (int i = numAdjacencies - remaining; i < numAdjacencies; i++) {
		adjacencyPtr[i].createEdges(vertices, i * 3);
	}

#if _DEBUG
	Timer::end(start, "Created adjacencies: ");
#endif
}

void MeshStriper::updateLink(AdjTriangle* triangles, int firstTri, int secondTri, uint16_t vertex0, uint16_t vertex1)
{
	AdjTriangle* tri0 = &triangles[firstTri];
	AdjTriangle* tri1 = &triangles[secondTri];
	int tri0EdgeIndex = tri0->getEdgeIndex(vertex0, vertex1);
	int tri1EdgeIndex = tri1->getEdgeIndex(vertex0, vertex1);
	tri0->adjacentTris[tri0EdgeIndex] = secondTri;
	tri1->adjacentTris[tri1EdgeIndex] = firstTri;
}

void MeshStriper::updateLink(AdjTriangle* triangles, int firstTri, int secondTri, uint32_t edge)
{
	AdjTriangle* tri0 = &triangles[firstTri];
	AdjTriangle* tri1 = &triangles[secondTri];
	int tri0EdgeIndex = tri0->getEdgeIndex(edge);
	int tri1EdgeIndex = tri1->getEdgeIndex(edge);
	tri0->adjacentTris[tri0EdgeIndex] = secondTri;
	tri1->adjacentTris[tri1EdgeIndex] = firstTri;
}

void MeshStriper::linkTriangleStructures(std::vector<AdjTriangle>& adjacencies)
{
#if _DEBUG
	auto start = Timer::begin();
#endif

	int edgeCount = (int)adjacencies.size() * 3;
	std::vector<int> faceIndices(edgeCount); // every edge has an associated face
	std::vector<uint16_t> firstVertices(edgeCount);
	std::vector<uint16_t> secondVertices(edgeCount);

	AdjTriangle* adjacencyPtr = adjacencies.data();
	int* faceIndicesPtr = faceIndices.data();
	uint16_t* firstVerticesPtr = firstVertices.data();
	uint16_t* secondVerticesPtr = secondVertices.data();

	for (int i = 0; i < adjacencies.size(); i++) {
		int edgeIndex = i * 3;
		faceIndicesPtr[edgeIndex] = i;
		faceIndicesPtr[edgeIndex + 1] = i;
		faceIndicesPtr[edgeIndex + 2] = i;
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
	Sorter sorter;
	std::vector<int> firstSortedIndices(edgeCount);
	std::vector<int> secondSortedIndices(edgeCount);
	sorter.sortFast(firstVertices, firstSortedIndices);
	sorter.sortFast(secondVertices, firstSortedIndices, secondSortedIndices);

	// Read the list in sorted order, creating links between adjacent triangles
	int* sortedIndicesPtr = secondSortedIndices.data();
	int sortedIndex = sortedIndicesPtr[0];
	uint16_t lastVertex0 = firstVerticesPtr[sortedIndex];
	uint16_t lastVertex1 = secondVerticesPtr[sortedIndex];
	uint32_t combinedLastVertex = (lastVertex0 << 16) | lastVertex1;
	char count = 0;
	int faces[2];

	for (int i = 0; i < edgeCount; i++) {
		sortedIndex = sortedIndicesPtr[i];
		int faceIndex = faceIndicesPtr[sortedIndex];
		uint16_t vertex0 = firstVerticesPtr[sortedIndex];
		uint16_t vertex1 = secondVerticesPtr[sortedIndex];
		uint32_t combinedCurrentVertex = (vertex0 << 16) | vertex1;
		// vertex0 == lastVertex0 && vertex1 == lastVertex1
		if (combinedCurrentVertex == combinedLastVertex) {
			faces[count] = faceIndex;
			count++;
		}
		else {
			if (count == 2) updateLink(adjacencyPtr, faces[0], faces[1], combinedLastVertex);
			// Reset for next edge
			faces[0] = faceIndex;
			count = 1;
			combinedLastVertex = combinedCurrentVertex;
		}
	}
	if (count == 2) updateLink(adjacencyPtr, faces[0], faces[1], combinedLastVertex);

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

void MeshStriper::generateStrips(std::vector<AdjTriangle>& adjacencies, int numTriangles, std::vector<std::vector<uint16_t>>& strips)
{
	auto start = Timer::begin();
	std::vector<int> indices(numTriangles);
	std::iota(indices.begin(), indices.end(), 0);
	int remainingTriangles = numTriangles;
	AdjTriangle* triangles = adjacencies.data();
	int* indicesPtr = indices.data();
	ProgressBar progressBar(numTriangles);
	progressBar.start();

	int lastTriangleIndex = 0;
	while (remainingTriangles > 0) {
		strips.emplace_back();
		std::vector<uint16_t>* strip = &strips.back();
		strip->resize(3);
		uint16_t* stripData = strip->data();
		int nextTriangleIndex = -1;
		for (int i = lastTriangleIndex; i < numTriangles; i++) {
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
		remainingTriangles--;
		memcpy(stripData, firstTri->vertices, 3 * sizeof(uint16_t)); // move current triangle vertices into start of strip
		uint16_t firstVertex = stripData[0];
		uint16_t secondVertex = stripData[1];
		uint16_t thirdVertex = stripData[2];
		uint16_t secondVertexOld = secondVertex;
		while (true) {
			int frontEdgeIndex;
			if (secondVertex < thirdVertex) frontEdgeIndex = currentFrontTri->getEdgeIndex(secondVertex, thirdVertex);
			else frontEdgeIndex = currentFrontTri->getEdgeIndex(thirdVertex, secondVertex);
			int adjacentTriIndex = currentFrontTri->adjacentTris[frontEdgeIndex];
			if (indicesPtr[adjacentTriIndex] != -1) {
				indicesPtr[adjacentTriIndex] = -1;
				remainingTriangles--;
				AdjTriangle* adjacentTri = &triangles[adjacentTriIndex];
				currentFrontTri = adjacentTri;
				uint16_t newVertex = adjacentTri->getOppositeVertex(secondVertex, thirdVertex);
				strip->emplace_back(newVertex);
				secondVertex = thirdVertex;
				secondVertexOld = secondVertex;
				thirdVertex = newVertex;
				continue;
			}
			else {
				stripData = strip->data();
				secondVertex = stripData[1];
				int backEdgeIndex;
				if (firstVertex < secondVertex) backEdgeIndex = currentBackTri->getEdgeIndex(firstVertex, secondVertex);
				else backEdgeIndex = currentBackTri->getEdgeIndex(secondVertex, firstVertex);
				adjacentTriIndex = currentBackTri->adjacentTris[backEdgeIndex];
				if (indicesPtr[adjacentTriIndex] == -1) break;
				indicesPtr[adjacentTriIndex] = -1;
				remainingTriangles--;
				AdjTriangle* adjacentTri = &triangles[adjacentTriIndex];
				currentBackTri = adjacentTri;
				uint16_t newVertex = adjacentTri->getOppositeVertex(firstVertex, secondVertex);
				strip->insert(strip->begin(), newVertex);
				stripData = strip->data();
				secondVertex = stripData[strip->size() - 2];
				firstVertex = newVertex;
			}
		}
		progressBar.updateProgress(numTriangles - remainingTriangles);
	}
	Timer::end(start, "Found (" + std::to_string(strips.size()) + ") triangle strips: ");
}

void MeshStriper::striper(FbxMesh* inMesh, MeshObject* outMesh)
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
	createTriangleStructures(adjacencies, vertices);
	linkTriangleStructures(adjacencies);
	generateStrips(adjacencies, triangleCount, outMesh->triangleStrips);

#if _DEBUG
	int memoryUsage = 0;
	memoryUsage += triangleCount * 36;
	std::cout << "Striper memory usage: " << memoryUsage << " bytes\n";
#endif
}
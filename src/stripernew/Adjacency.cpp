#include "Stdafx.h"
#include <iostream>

Adjacencies::~Adjacencies()
{
	RELEASEARRAY(edges);
	RELEASEARRAY(faces);
}

bool Adjacencies::init(AdjacenciesCreate& create)
{
	//auto start = Timer::begin();
	// Allocate ram for triangles and edges. Might fail, so use null checks.
	faceCount = create.faceCount;
	faces = new AdjTriangle[faceCount];
	if(!faces) return false;
	edges = new AdjEdge[faceCount * 3];
	if(!edges) return false;

	// Get vertex indices for each triangle and add to database
	for(uint32_t i = 0; i < faceCount; i++)
	{
		uint32_t vertex0 = create.DFaces ? create.DFaces[i*3+0] : create.WFaces ? create.WFaces[i*3+0] : 0;
		uint32_t vertex1 = create.DFaces ? create.DFaces[i*3+1] : create.WFaces ? create.WFaces[i*3+1] : 1;
		uint32_t vertex2 = create.DFaces ? create.DFaces[i*3+2] : create.WFaces ? create.WFaces[i*3+2] : 2;
		addTriangle(vertex0, vertex1, vertex2);
	}
	//Timer::end(start, "Created adjaceny list: ");
	return true;
}

bool Adjacencies::addTriangle(uint32_t vertex0, uint32_t vertex1, uint32_t vertex3)
{
	// Store vertex-references
	faces[currentFaceIndex].vertexIndices[0] = vertex0;
	faces[currentFaceIndex].vertexIndices[1] = vertex1;
	faces[currentFaceIndex].vertexIndices[2] = vertex3;

	// Reset links
	faces[currentFaceIndex].adjacentTris[0]	= -1;
	faces[currentFaceIndex].adjacentTris[1]	= -1;
	faces[currentFaceIndex].adjacentTris[2]	= -1;

	if(vertex0 < vertex1) addEdge(vertex0, vertex1, currentFaceIndex); // edge 0-1
	else addEdge(vertex1, vertex0, currentFaceIndex); // edge 1-0
	if(vertex0 < vertex3) addEdge(vertex0, vertex3, currentFaceIndex); // edge 1-2
	else addEdge(vertex3, vertex0, currentFaceIndex); // edge 2-1
	if(vertex1 < vertex3) addEdge(vertex1, vertex3, currentFaceIndex); // edge 2-0
	else addEdge(vertex3, vertex1, currentFaceIndex); // edge 0-2

	currentFaceIndex++;

	return true;
}

bool Adjacencies::addEdge(uint32_t vertex0, uint32_t vertex1, uint32_t faceIndex)
{
	// Store edge data
	edges[edgeCount].vertex0 = vertex0;
	edges[edgeCount].vertex1 = vertex1;
	edges[edgeCount].faceIndex = faceIndex;
	edgeCount++;
	return true;
}

bool Adjacencies::createDatabase()
{
	// Here edgeCount should be equal to currentFaceIndex*3.

	RadixSorter Core;

	uint32_t* faceIndices = new uint32_t[edgeCount]; if(!faceIndices) return false;
	uint32_t* firstVertexIndices = new uint32_t[edgeCount]; if(!firstVertexIndices) return false;
	uint32_t* secondVertexIndices = new uint32_t[edgeCount]; if(!secondVertexIndices) return false;

	for(uint32_t i = 0; i < edgeCount; i++) {
		faceIndices[i] = edges[i].faceIndex;
		firstVertexIndices[i] = edges[i].vertex0;
		secondVertexIndices[i] = edges[i].vertex1;
	}

	// Multiple sort
	uint32_t* sorted = Core
		.sort(faceIndices, edgeCount)
		.sort(firstVertexIndices, edgeCount)
		.sort(secondVertexIndices, edgeCount)
		.getIndices();

	// Read the list in sorted order, look for similar edges
	uint32_t lastVertex0 = firstVertexIndices[sorted[0]];
	uint32_t lastVertex1 = secondVertexIndices[sorted[0]];
	uint32_t count = 0;
	uint32_t tmpBuffer[3];

	for(int i = 0; i < edgeCount; i++) {
		uint32_t face = faceIndices[sorted[i]]; // Owner face
		uint32_t vertex0 = firstVertexIndices[sorted[i]]; // Vertex ref #1
		uint32_t vertex1 = secondVertexIndices[sorted[i]]; // Vertex ref #2
		if(vertex0 == lastVertex0 && vertex1 == lastVertex1) {
			// Current edge is the same as last one
			tmpBuffer[count++] = face; // Store face number
			if(count == 3) {
				RELEASEARRAY(secondVertexIndices);
				RELEASEARRAY(firstVertexIndices);
				RELEASEARRAY(faceIndices);
				return false; // Only works with manifold meshes (i.e. an edge is not shared by more than 2 triangles)
			}
		} else {
			// Here we have a new edge (lastVertex0, lastVertex1) shared by count triangles stored in tmpBuffer
			if(count == 2) {
				// if count == 1 => edge is a boundary edge: it belongs to a single triangle.
				// Hence there's no need to update a link to an adjacent triangle.
				bool status = updateLink(tmpBuffer[0], tmpBuffer[1], lastVertex0, lastVertex1);
				if(!status)
				{
					RELEASEARRAY(secondVertexIndices);
					RELEASEARRAY(firstVertexIndices);
					RELEASEARRAY(faceIndices);
					return status;
				}
			}
			// Reset for next edge
			count = 0;
			tmpBuffer[count++] = face;
			lastVertex0 = vertex0;
			lastVertex1 = vertex1;
		}
	}
	bool status = true;
	if(count == 2) status = updateLink(tmpBuffer[0], tmpBuffer[1], lastVertex0, lastVertex1);

	RELEASEARRAY(secondVertexIndices);
	RELEASEARRAY(firstVertexIndices);
	RELEASEARRAY(faceIndices);

	// We don't need the edges anymore
	RELEASEARRAY(edges);

	return status;
}

bool Adjacencies::updateLink(uint32_t firstTri, uint32_t secondTri, uint32_t vertex0, uint32_t vertex1)
{
	AdjTriangle* tri0 = &faces[firstTri];		// Catch the first triangle
	AdjTriangle* tri1 = &faces[secondTri];		// Catch the second triangle

	// Get the edge IDs. 0xff means input references are wrong.
	uint8_t EdgeNb0 = tri0->findEdge(vertex0, vertex1);
	if(EdgeNb0==0xff) return false;
	uint8_t EdgeNb1 = tri1->findEdge(vertex0, vertex1);
	if(EdgeNb1==0xff) return false;

	// Update links. The two most significant bits contain the counterpart edge's ID.
	tri0->adjacentTris[EdgeNb0] = secondTri |(uint32_t(EdgeNb1)<<30);
	tri1->adjacentTris[EdgeNb1] = firstTri |(uint32_t(EdgeNb0)<<30);

	return true;
}

uint8_t AdjTriangle::findEdge(uint32_t vertex0, uint32_t vertex1)
{
	uint8_t edgeIndex = 0xff;
	if(vertexIndices[0]==vertex0 && vertexIndices[1]==vertex1) edgeIndex = 0;
	else if(vertexIndices[0]==vertex1 && vertexIndices[1]==vertex0) edgeIndex = 0;
	else if(vertexIndices[0]==vertex0 && vertexIndices[2]==vertex1) edgeIndex = 1;
	else if(vertexIndices[0]==vertex1 && vertexIndices[2]==vertex0) edgeIndex = 1;
	else if(vertexIndices[1]==vertex0 && vertexIndices[2]==vertex1) edgeIndex = 2;
	else if(vertexIndices[1]==vertex1 && vertexIndices[2]==vertex0) edgeIndex = 2;
	return edgeIndex;
}

/// <summary>
/// Given two vertex indices, get the remaining vertex index
/// </summary>
/// <param name="vertex0"></param>
/// <param name="vertex1"></param>
/// <returns></returns>
uint32_t AdjTriangle::oppositeVertex(uint32_t vertex0, uint32_t vertex1)
{
	uint32_t vertexIndex = 0xffffffff;
	if(vertexIndices[0]==vertex0 && vertexIndices[1]==vertex1) vertexIndex = vertexIndices[2];
	else if(vertexIndices[0]==vertex1 && vertexIndices[1]==vertex0) vertexIndex = vertexIndices[2];
	else if(vertexIndices[0]==vertex0 && vertexIndices[2]==vertex1) vertexIndex = vertexIndices[1];
	else if(vertexIndices[0]==vertex1 && vertexIndices[2]==vertex0) vertexIndex = vertexIndices[1];
	else if(vertexIndices[1]==vertex0 && vertexIndices[2]==vertex1) vertexIndex = vertexIndices[0];
	else if(vertexIndices[1]==vertex1 && vertexIndices[2]==vertex0) vertexIndex = vertexIndices[0];
	return vertexIndex;
}

#include <striper/TriangleStripGenerator.h>
#include <model/MeshObject.h>
#include <util/Timer.hpp>
#include <string>
#include <numeric>
#include <chrono>
#include <fbxsdk.h>
#include <algorithm>

/// <summary>
/// Compare 2 triangles, (x, y, z) and (a, b, c). If both triangles share 2 indices then they share an edge, and are considered adjacent.
/// Return the index that the second triangle doesn't share. This will be used as the next vertex in the triangle strip.
/// </summary>
/// <param name="x"></param>
/// <param name="y"></param>
/// <param name="z"></param>
/// <param name="a"></param>
/// <param name="b"></param>
/// <param name="c"></param>
/// <returns></returns>
int isAdjacentFront(int y, int z, int a, int b, int c) {

	bool b1 = false;
	bool b2 = false;
	bool b3 = false;
	if (y == a) {
		b1 = true;
	}
	else if (y == b) {
		b2 = true;
	}
	else if (y == c) {
		b3 = true;
	}
	else {
		// If the y element isn't equal to a, b or c, then it is guaranteed that triangle a,b,c is not adjacent to x,y,z
		return -1;
	}

	/*
	* Check the flags first to avoid doing unecessary equality checks.
	* Example: If the y equals the a, then we don't need to check if the z equals the a, so skip it.
	*/
	if (!b1 && z == a) {
		b1 = true;
	}
	else if (!b2 && z == b) {
		b2 = true;
	}
	else if (!b3 && z == c) {
		b3 = true;
	}
	else {
		// If the z element isn't equal to a, b or c, then it is guaranteed that triangle a,b,c is not adjacent to x,y,z
		return -1;
	}
	// when 2 indices are equal, the flag for the second index is set to true.
	// if 2 pair of indices are found, then the triangles share an edge and are therefore adjacent.
	// so return the flag that isn't true, and return the corresponding index from the second triangle.
	if (!b1) return a;
	if (!b2) return b;
	if (!b3) return c;

	return -1;
}

/// <summary>
/// Same thing as isAdjacentFront, except we check the first two indices of against the second triangle instead of the last 2.
/// Example: for triangles (x, y, z) and (a, b, c), instead of doing checks for y and z against (a, b, c), do checks for x and y against (a, b, c)
/// Return the index that the second triangle doesn't share. This will be used as the next vertex in the triangle strip.
/// </summary>
/// <param name="x"></param>
/// <param name="y"></param>
/// <param name="z"></param>
/// <param name="a"></param>
/// <param name="b"></param>
/// <param name="c"></param>
/// <returns></returns>
int isAdjacentBehind(int x, int y, int a, int b, int c) {

	bool b1 = false;
	bool b2 = false;
	bool b3 = false;
	if (x == a) {
		b1 = true;
	}
	else if (x == b) {
		b2 = true;
	}
	else if (x == c) {
		b3 = true;
	}
	else {
		// If the x element isn't equal to a, b or c, then it is guaranteed that triangle a,b,c is not adjacent to x,y,z
		return -1;
	}

	if (!b1 && y == a) {
		b1 = true;
	}
	else if (!b2 && y == b) {
		b2 = true;
	}
	else if (!b3 && y == c) {
		b3 = true;
	}
	else {
		// If the y element isn't equal to a, b or c, then it is guaranteed that triangle a,b,c is not adjacent to x,y,z
		return -1;
	}
	if (!b1) return a;
	if (!b2) return b;
	if (!b3) return c;

	return -1;
}

void striper(FbxMesh* inMesh, MeshObject* outMesh) {
	int polygonCount;
#if _DEBUG
	auto readStart = Timer::begin();
	polygonCount = inMesh->GetPolygonCount();
	Timer::end(readStart, "Found (" + std::to_string(polygonCount) + ") triangles: ");
#endif
	auto start = Timer::begin();
	polygonCount = inMesh->GetPolygonCount();
	std::vector<int> triangles(polygonCount);
	std::iota(triangles.begin(), triangles.end(), 0);
	std::vector<uint16_t> currentStrip;
	int sizeondisk = 0;

	// array indexes are slow. use pointers instead
	int* vertices = inMesh->GetPolygonVertices();
	int* trianglesPtr = triangles.data();

	// for progress bar
	std::cout << "0";
	int previousCompletion = 0;
	int completion = 0;
	const char* progessChars = "...1...2...3...4...5...6...7...8...9...1";

	while (true) {
		int firstTriangleIndex = trianglesPtr[0] * 3;
		int frontX = vertices[firstTriangleIndex];
		int frontY = vertices[firstTriangleIndex + 1];
		int frontZ = vertices[firstTriangleIndex + 2];
		int backX = frontX;
		int backY = frontY;
		currentStrip.emplace_back(frontX);
		currentStrip.emplace_back(frontY);
		currentStrip.emplace_back(frontZ);
		triangles.erase(triangles.begin());

		auto passStart = Timer::begin();
		int numTriangles = (int)triangles.size();
		while (true) {
			int adjacentTris = 0;
			for (int i = 0; i < numTriangles; ++i) {
				int triangleIndex = trianglesPtr[i] * 3; // array indexes are slow. use pointer access instead
				int v1 = vertices[triangleIndex];
				int v2 = vertices[triangleIndex + 1];
				int v3 = vertices[triangleIndex + 2];

				int adjacentFront = isAdjacentFront(frontY, frontZ, v1, v2, v3);
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
				int adjacentBehind = isAdjacentBehind(backX, backY, v1, v2, v3);
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

		// progress bar
		completion = (1 - ((float)triangles.size() / (float)polygonCount)) * 40; // value between 1 and 40
		int diff = completion - previousCompletion;
		if (diff) {
			for (int i = 0; i < diff; ++i)
				std::cout << progessChars[previousCompletion + i];
			previousCompletion = completion;
		}

		if (triangles.empty()) break; // no more triangles mean we have created all the strips needed.
	}
	std::cout << "0" << std::endl;
	Timer::end(start, "[MODELMAKER] Generated (" + std::to_string(outMesh->triangleStrips.size()) + ") triangle strips: ");
#if _DEBUG
	std::cout << "Size on disk: " << sizeondisk << " bytes\n";
#endif
}

int getRemainingIndex(uint32_t edge, uint32_t edgeA, uint32_t edgeB, uint32_t edgeC, uint64_t vertices) {
	if (edge == edgeA) return (uint16_t)vertices;
	if (edge == edgeB) return (uint16_t)(vertices >> 32);
	if (edge == edgeC) return (uint16_t)(vertices >> 16);
	return -1;
}

void striperNew(FbxMesh* inMesh, MeshObject* outMesh) {
	auto start = Timer::begin();
	int polygonCount = inMesh->GetPolygonCount();
	std::vector<uint16_t> currentStrip;
	std::vector<uint16_t> triangleIndices(polygonCount);
	std::vector<uint64_t> vertexIndices(polygonCount);

	// array indexes are slow. use pointer access instead
	int* vertexPointer = inMesh->GetPolygonVertices();
	uint16_t* triangleIndicesPtr = triangleIndices.data();
	uint32_t* edgePointer = outMesh->edges.data();
	uint64_t* vertexIndicesPtr = vertexIndices.data();

	std::iota(triangleIndices.begin(), triangleIndices.end(), 0);
	for (int i = 0; i < polygonCount; ++i) {
		int startIndex = i * 3;
		vertexIndicesPtr[i] = ((uint64_t)vertexPointer[startIndex] << 32) | ((uint64_t)vertexPointer[startIndex + 1] << 16) | (uint64_t)vertexPointer[startIndex + 2];
	}
	int sizeondisk = 0;
	int remainingTriangles = polygonCount;

	// for progress bar
	std::cout << "0";
	int previousCompletion = 0;
	int completion = 0;
	const char* progessChars = "...1...2...3...4...5...6...7...8...9...1";

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

		// progress bar
		completion = (1 - ((float)triangleIndices.size() / (float)polygonCount)) * 40; // value between 1 and 40
		int diff = completion - previousCompletion;
		if (diff) {
			for (int i = 0; i < diff; ++i)
				std::cout << progessChars[previousCompletion + i];
			previousCompletion = completion;
		}

		if (triangleIndices.empty()) break;
	}
	std::cout << "0" << std::endl;
	Timer::end(start, "[MODELMAKER] Generated (" + std::to_string(outMesh->triangleStrips.size()) + ") strips: ");
#if _DEBUG
	std::cout << "Size on disk: " << sizeondisk << " bytes\n";
#endif
}
#ifndef SRC_MESHSTRIPER_MESHSTRIPER_H_
#define SRC_MESHSTRIPER_MESHSTRIPER_H_

#include <fbxsdk.h>
#include <model/MeshObject.h>

struct Edge {
	uint16_t v1;
	uint16_t v2;
};

struct AdjTriangle {
	Edge edges[3];
	int adjacentTris[3] = { -1, -1, -1 };
	uint16_t vertices[3];

	/// <summary>
	/// Populate the edges array of a triangle
	/// </summary>
	/// <param name="v1"> - first vertex</param>
	/// <param name="v2"> - second vertex</param>
	/// <param name="v3"> - third vertex</param>
	void createEdges(int v1, int v2, int v3);

	/// <summary>
	/// <para/>Return the index of the edge formed by the given vertices.
	/// <para/>v1 must be less than v2 for this method to work.
	/// </summary>
	/// <param name="v1"> - first vertex</param>
	/// <param name="v2"> - second vertex</param>
	/// <returns></returns>
	int getEdgeIndex(uint16_t v1, uint16_t v2);

	/// <summary>
	/// Given two vertices, return the third vertex in the triangle.
	/// </summary>
	/// <param name="v1"> - first vertex</param>
	/// <param name="v2"> - second vertex</param>
	/// <returns></returns>
	int getOppositeVertex(uint16_t v1, uint16_t v2);
};

class MeshStriper {
private:
	/// <summary>
	/// For each triangle, create an AdjTriangle struct with 3 edges and 3 vertices
	/// </summary>
	/// <param name="triangles">- array to populate with triangles</param>
	/// <param name="vertices">- array of vertices to create triangles from</param>
	void createTriangleStructures(std::vector<AdjTriangle>& triangles, int* vertices);

	/// <summary>
	/// <para/>Create a link between two given triangles by updating their respective adjacency structures.
	/// <para/>Each triangle has an array of adjacent triangles, and an array of edges. The adjacent triangles and edges map 1-1.
	/// <para/>If seondTri is adjacent to firstTri along edge 0 of firstTri, then secondTri will be stored at index 0 in firsTri's adjacent tris array and vice verse.
	/// </summary>
	/// <param name="triangles">- array of triangles</param>
	/// <param name="firstTri">- first triangle</param>
	/// <param name="secondTri">- second triangle</param>
	/// <param name="vertex0">- first vertex, used to get the shared edge between firstTri and secondTri</param>
	/// <param name="vertex1">- second vertex, used to get the shared edge between firstTri and secondTri</param>
	/// <returns></returns>
	void updateLink(AdjTriangle* triangles, int firstTri, int secondTri, uint16_t vertex0, uint16_t vertex1);

	/// <summary>
	/// <para/>Link the adjacency structures by creating a list of all edges in the mesh and sorting by the second vertex of each edge.
	/// <para/>This creates a list of indexes, each index leading to an element in the unsorted list.
	/// <para/>For example, if the unsorted edges are [4, 8, 2, 6, 3, 2], and the sorted indexes are [2, 5, 4, 0, 3, 1],
	/// then indexing the unsorted edges using the sorted index array would produce [2, 2, 3, 4, 6, 8].
	/// <para/>The sorted index array can then be used to obtain the first vertex, second vertex, and face index for
	/// every edge.
	/// <para/>By looping through the sorted index array you can quickly identify matching edges and create the corresponding links between triangles.
	/// </summary>
	/// <param name="triangles">- array of triangles</param>
	void linkTriangleStructures(std::vector<AdjTriangle>& triangles);

	/// <summary>
	/// <para/>Generate triangle strips by walking through adjacency structures until no more adjacent triangles can be found.
	/// <para/>Strips are extended both forwards and backwards to maximise their lengths.
	/// </summary>
	/// <param name="triangles">- array of triangles</param>
	/// <param name="numTriangles">- number of triangles in array</param>
	/// <param name="strips">- array to put strips into</param>
	void generateStrips(std::vector<AdjTriangle>& triangles, int numTriangles, std::vector<std::vector<uint16_t>>& strips);
public:
	/// <summary>
	/// <para/>Converts a triangulated mesh into an array of triangle strips.
	/// <para/>The strips are stored in outMesh.triangleStrips
	/// </summary>
	/// <param name="inMesh">- fbx mesh</param>
	/// <param name="outMesh">- output mesh to put triangle strips into</param>
	void striper(FbxMesh* inMesh, MeshObject* outMesh);
};

#endif
#ifndef SRC_MODEL_FBXREADER_H_
#define SRC_MODEL_FBXREADER_H_

#include <vector>
#include <fbxsdk.h>
#include <model/MeshObject.h>

class FBXReader {
public:
	/// <summary>
	/// Read an FBX file
	/// FBX file must have only one object which contains one mesh
	/// This only works with vertices that use whole numbers between -32768 and 32768.
	/// If the file contains bigger numbers, or decimals vertices, you can kiss those numbers goodbye.
	/// Reason is, each vertex position is stored as 2 bytes in a .m file, which reduces file size, so signed ints work best, as long as they are within 2 bytes.
	/// </summary>
	/// <param name="path"></param>
	/// <returns>Read success</returns>
	static bool readFBXModel(const char* path, MeshObject* outMesh);
private:
	/// <summary>
	/// Loop through vertices and stick 'em into the vertex vector
	/// </summary>
	/// <param name="mesh"></param>
	/// <param name="outVertices"></param>
	static void readFBXVertices(FbxMesh* mesh, MeshObject* outMesh);

	/// <summary>
	/// Loop through the polygons and generate triangle strips, uv strips and a normal list
	/// </summary>
	/// <param name="mesh"></param>
	/// <param name="outTriangles"></param>
	static void readFBXTriangles(FbxMesh* mesh, MeshObject* outMesh);

	/// <summary>
	/// Read uv coords for each triangle.
	/// </summary>
	/// <param name="mesh"></param>
	/// <param name="outMesh"></param>
	static void readFBXUVs(FbxMesh* mesh, MeshObject* outMesh);
};

#endif
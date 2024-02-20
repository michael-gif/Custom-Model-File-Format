#ifndef SRC_MODEL_MODELMANAGER_H_
#define SRC_MODEL_MODELMANAGER_H_

#include <model/MeshObject.h>
#include <string>
#include <fstream>
#include <vector>
#include <fbxsdk.h>

class ModelManager {
public:
	/// <summary>
	/// Read model file
	/// </summary>
	/// <param name="path"></param>
	/// <returns>Pointer to loaded mesh object</returns>
	static void readModel(const char* path, MeshObject* outMesh);

	/// <summary>
	/// Write MeshObject to .m file
	/// </summary>
	/// <param name="mesh"></param>
	/// <param name="filename"></param>
	static void writeToDisk(MeshObject* mesh, std::string filename);
	static void compare(MeshObject* meshA, MeshObject* meshB);
private:
	/// <summary>
	/// File is read in chunks to reduce overhead from file::read. Maximum chunksize seems to be 64, meaning 64 vertices are read at a time.
	/// The remaining vertices are read 1 by 1 at the end.
	/// Each vertex is 3 floats, so 12 bytes a vertex.
	/// </summary>
	/// <param name="file"></param>
	/// <returns></returns>
	static void readVertices(std::ifstream& file, MeshObject* mesh);

	/// <summary>
	/// Read triangle strips in chunks to reduce overhead from file::read. Maximum chunk size seems to be 64, meaning 64 vertices are read at a time.
	/// Each vertex index is 2 bytes.
	/// </summary>
	/// <param name="file"></param>
	/// <returns></returns>
	static void readTriangleStrips(std::ifstream& file, MeshObject* mesh);

	/// <summary>
	/// Read UV coords in chunks to reduce overhead from file::read. Maximum chunk size seems to be 256;
	/// Remaing uvs are read 1 by 1 at the end.
	/// Each uv is 2 bytes.
	/// </summary>
	/// <param name="file"></param>
	/// <param name="outCoords"></param>
	static void readUVs(std::ifstream& file, MeshObject* mesh);

	/// <summary>
	/// Read vertex normals in chunks to reduce overhead from file::read. Maximum chunk size seems to be 64, meaning 64 normals are read at a time.
	/// Remaining normls are read 1 by 1 at the end.
	/// Each normal is 3 floats, so 12 bytes a normal.
	/// </summary>
	/// <param name="file"></param>
	/// <param name="mesh"></param>
	static void readVertexNormals(std::ifstream& file, MeshObject* mesh);

	/// <summary>
	/// Each vertex is represented as 3 floats of 4 bytes each, for the x, y, and z, so 12 bytes per vertex.
	/// The vertex bytes are stored directly next to each other, with a vertex count at the start.
	/// Each vertex has a UV coordinate. A coord is 2 floats, 4 bytes each, so 8 bytes per uv coord.
	/// UV coordinate bytes are stored after vertex bytes.
	/// 
	/// There is a maximum of 65,536 vertices, since the triangles use 2 bytes per vertex index.
	/// If you want more vertices, the triangles will have to use 3 or 4 bytes per index, which will increase file size.
	/// </summary>
	/// <param name="mesh"></param>
	/// <param name="file"></param>
	static void writeVertices(MeshObject* mesh, std::ofstream& file);

	/// <summary>
	/// Write triangle strips
	/// </summary>
	/// <param name="mesh"></param>
	/// <param name="file"></param>
	static void writeTriangleStrips(MeshObject* mesh, std::ofstream& file);

	/// <summary>
	/// Write UV coord strips
	/// </summary>
	/// <param name="mesh"></param>
	/// <param name="file"></param>
	static void writeUVs(MeshObject* mesh, std::ofstream& file);

	/// <summary>
	/// Write vertex normals. 12 bytes a vertex.
	/// </summary>
	/// <param name="mesh"></param>
	/// <param name="file"></param>
	static void writeVertexNormals(MeshObject* mesh, std::ofstream& file);
};

#endif
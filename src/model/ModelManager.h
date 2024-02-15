#ifndef SRC_MODEL_MODELMANAGER_H_
#define SRC_MODEL_MODELMANAGER_H_

#include <model/MeshObject.h>
#include <string>
#include <fstream>
#include <vector>
#include <fbxsdk.h>

class ModelManager {
public:
	static void readModel(const char* path, MeshObject* outMesh);
	static void writeToDisk(MeshObject* mesh, std::string filename);
	static void compare(MeshObject* meshA, MeshObject* meshB);
private:
	static void readVertices(std::ifstream& file, MeshObject* mesh);
	static void readTriangleStrips(std::ifstream& file, MeshObject* mesh);
	static void readUVs(std::ifstream& file, MeshObject* mesh);
	static void readVertexNormals(std::ifstream& file, MeshObject* mesh);
	static void writeVertices(MeshObject* mesh, std::ofstream& file);
	static void writeTriangleStrips(MeshObject* mesh, std::ofstream& file);
	static void writeUVs(MeshObject* mesh, std::ofstream& file);
	static void writeVertexNormals(MeshObject* mesh, std::ofstream& file);
};

#endif
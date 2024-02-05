#pragma once
#ifndef MODEL_MANAGER_HPP
#define MODEL_MANAGER_HPP

#include <MeshObject.h>
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
	static void readNormals(std::ifstream& file, MeshObject* mesh);
	static void writeVertices(MeshObject* mesh, std::ofstream& file);
	static void writeTriangleStrips(MeshObject* mesh, std::ofstream& file);
	static void writeUVs(MeshObject* mesh, std::ofstream& file);
	static void writeNormals(MeshObject* mesh, std::ofstream& file);
};

#endif
#ifndef SRC_MODEL_FBXREADER_H_
#define SRC_MODEL_FBXREADER_H_

#include <vector>
#include <fbxsdk.h>
#include <model/MeshObject.h>

class FBXReader {
public:
	static bool readFBXModel(const char* path, MeshObject* outMesh);
private:
	static void readFBXVertices(FbxMesh* mesh, MeshObject* outMesh);
	static void readFBXTriangles(FbxMesh* mesh, MeshObject* outMesh);
	static void readFBXUVs(FbxMesh* mesh, MeshObject* outMesh);
};

#endif
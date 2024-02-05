#pragma once
#ifndef FBXREADER_H_
#define FBXREADER_H_

#include <vector>
#include <fbxsdk.h>
#include <MeshObject.h>

class FBXReader {
public:
	static bool readFBXModel(const char* path, MeshObject* outMesh);
	static void readFBXVertices(FbxMesh* mesh, MeshObject* outMesh);
	static void readFBXTriangles(FbxMesh* mesh, MeshObject* outMesh);
	static void readFBXUVs(FbxMesh* mesh, MeshObject* outMesh);
};

#endif
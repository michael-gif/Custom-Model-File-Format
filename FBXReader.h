#pragma once
#ifndef FBX_READER_HPP
#define FBX_READER_HPP

#include <vector>
#include <fbxsdk.h>
#include <MeshObject.h>

class FBXReader {
public:
	static MeshObject loadedFBXMesh;
	static MeshObject* readFBXModel(const char* path);
	static void readFBXVertices(FbxMesh* mesh, MeshObject* outMesh);
	static void readFBXTriangles(FbxMesh* mesh, MeshObject* outMesh);
};

#endif
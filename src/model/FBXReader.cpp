#include <iostream>
#include <string>
#include <chrono>
#include <numeric>
#include <fbxsdk.h>
#include <model/FBXReader.h>
#include <model/MeshObject.h>
#include <util/Timer.hpp>
#include <striper/TriangleStripGenerator.h>

/// <summary>
/// Read an FBX file
/// FBX file must have only one object which contains one mesh
/// This only works with vertices that use whole numbers between -32768 and 32768.
/// If the file contains bigger numbers, or decimals vertices, you can kiss those numbers goodbye.
/// Reason is, each vertex position is stored as 2 bytes in a .m file, which reduces file size, so signed ints work best, as long as they are within 2 bytes.
/// </summary>
/// <param name="path"></param>
/// <returns>Read success</returns>
bool FBXReader::readFBXModel(const char* path, MeshObject* outMesh)
{
#if _DEBUG
	auto start = Timer::begin();
#endif
	if (path == nullptr) {
		std::cout << "path is null" << std::endl;
		return false;
	}
	std::string strPath(path);
	size_t len = strPath.size();
	if (len == 0) {
		std::cout << "path is empty" << std::endl;
		return false;
	}
	else if (len < 4) {
		std::cout << "not an fbx file" << std::endl;
		return false;
	}
	else if (len >= 4) {
		if (strPath.substr(strPath.size() - 4) != ".fbx") {
			std::cout << "file doesn't end with .fbx" << std::endl;
			return false;
		}
	}
	FbxManager* manager = FbxManager::Create();
	FbxIOSettings* ioSettings = FbxIOSettings::Create(manager, IOSROOT);
	manager->SetIOSettings(ioSettings);
	FbxImporter* importer = FbxImporter::Create(manager, "");
	auto loadStart = Timer::begin();
	if (!importer->Initialize(path, -1, manager->GetIOSettings())) {
		std::cout << "Could not initialize fbx file" << std::endl;
		importer->Destroy();
		ioSettings->Destroy();
		manager->Destroy();
		return false;
	}
	FbxScene* scene = FbxScene::Create(manager, "MyScene");
	importer->Import(scene);
	importer->Destroy();

	auto triangulateStart = Timer::begin();
	FbxGeometryConverter converter(manager);
	converter.Triangulate(scene, true);
	Timer::end(triangulateStart, "Triangulated scene: ");

	FbxNode* rootNode = scene->GetRootNode();
	if (!rootNode) {
		std::cout << "Couldn't get root node\n";
		return false;
	}
	FbxNode* node = rootNode->GetChild(0);
#if _DEBUG
	Timer::end(start, "Initialized fbx scene: ");
	std::cout << "[FBX] Detected object: '" << node->GetName() << "'" << std::endl;
#endif
	FbxMesh* mesh = node->GetMesh();
	if (!mesh) {
		std::cout << "'" << node->GetName() << "' does not contain a mesh\n";
		scene->Destroy();
		ioSettings->Destroy();
		manager->Destroy();
		return false;
	}

	Timer::end(loadStart, "[FBX] Loaded model: ");

	auto convertStart = Timer::begin();
	readFBXVertices(mesh, outMesh);
	readFBXTriangles2(mesh, outMesh);
	readFBXUVs(mesh, outMesh);

	scene->Destroy();
	ioSettings->Destroy();
	manager->Destroy();

	std::flush(std::cout);
	Timer::end(convertStart, "[MODELMAKER] Converted model: ");
	return true;
}

/// <summary>
/// Loop through vertices and stick 'em into the vertex vector
/// </summary>
/// <param name="mesh"></param>
/// <param name="outVertices"></param>
void FBXReader::readFBXVertices(FbxMesh* mesh, MeshObject* outMesh)
{
#if _DEBUG
	auto start = Timer::begin();
#endif
	FbxVector4* vertices = mesh->GetControlPoints();
	int vertexCount = mesh->GetControlPointsCount();
	std::cout << "[FBX] Detected mesh: '" << mesh->GetName() << "' with vertex count (" << vertexCount << ")" << std::endl;
	for (int j = 0; j < vertexCount; ++j) {
		outMesh->vertices.emplace_back(vertices[j][0], vertices[j][1], vertices[j][2]);
	}
#if _DEBUG
	Timer::end(start, "Read (" + std::to_string(vertexCount) + ") vertices: ");
#endif
}

/// <summary>
/// Loop through the polygons and generate triangle strips, uv strips and a normal list
/// </summary>
/// <param name="mesh"></param>
/// <param name="outTriangles"></param>
void FBXReader::readFBXTriangles(FbxMesh* mesh, MeshObject* outMesh)
{
	std::cout << "[MODELMAKER] Converting..." << std::endl;
	Striper striper;
	striper.striper(mesh, outMesh);
}

void FBXReader::readFBXTriangles2(FbxMesh* mesh, MeshObject* outMesh)
{
	std::cout << "[MODELMAKER] Converting..." << std::endl;
	Striper striper;
	striper.striper2(mesh, outMesh);
}

/// <summary>
/// Read uv coords for each triangle.
/// </summary>
/// <param name="mesh"></param>
/// <param name="outMesh"></param>
void FBXReader::readFBXUVs(FbxMesh* mesh, MeshObject* outMesh)
{
#if _DEBUG
	auto start = Timer::begin();
#endif
	FbxStringList uvSetNames;
	mesh->GetUVSetNames(uvSetNames);
	FbxString uvMapName = uvSetNames[0];
	FbxVector2 uvCoord;
	bool unmapped;
	outMesh->uvs.resize(mesh->GetPolygonCount() * 6);
	float* uvs = outMesh->uvs.data();

	for (int i = 0; i < mesh->GetPolygonCount(); ++i) {
		int startIndex = i * 6;
		mesh->GetPolygonVertexUV(i, 0, uvMapName, uvCoord, unmapped);
		uvs[i + 0] = (float)uvCoord[0];
		uvs[i + 1] = (float)uvCoord[1];
		mesh->GetPolygonVertexUV(i, 1, uvMapName, uvCoord, unmapped);
		uvs[i + 2] = (float)uvCoord[0];
		uvs[i + 3] = (float)uvCoord[1];
		mesh->GetPolygonVertexUV(i, 2, uvMapName, uvCoord, unmapped);
		uvs[i + 4] = (float)uvCoord[0];
		uvs[i + 5] = (float)uvCoord[1];
	}
#if _DEBUG
	Timer::end(start, "Read (" + std::to_string(outMesh->uvs.size()) + ") uv's: ");
#endif
}
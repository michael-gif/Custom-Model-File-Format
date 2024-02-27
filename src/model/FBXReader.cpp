#include <iostream>
#include <string>
#include <model/FBXReader.h>
#include <meshstriper/MeshStriper.h>
#include <util/Timer.hpp>

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
		manager->Destroy();
		return false;
	}

	Timer::end(loadStart, "[FBX] Loaded model: ");

	auto convertStart = Timer::begin();
	readFBXVertices(mesh, outMesh);
	readFBXTriangles(mesh, outMesh);
	readFBXUVs(mesh, outMesh);

	scene->Destroy();
	manager->Destroy();

	std::flush(std::cout);
	Timer::end(convertStart, "[MODELMAKER] Converted model: ");
	return true;
}

void FBXReader::readFBXVertices(FbxMesh* mesh, MeshObject* outMesh)
{
#if _DEBUG
	auto start = Timer::begin();
#endif

	int vertexCount = mesh->GetControlPointsCount();
	std::cout << "[FBX] Detected mesh: '" << mesh->GetName() << "' with vertex count (" << vertexCount << ")" << std::endl;
	FbxVector4* fbxVertices = mesh->GetControlPoints();
	outMesh->vertices.reserve(vertexCount);
	MeshObject::Vertex* meshVertices = outMesh->vertices.data();
	for (int j = 0; j < vertexCount; ++j) {
		FbxDouble* vertex = fbxVertices[j].mData;
		meshVertices[j].setPos((float)vertex[0], (float)vertex[1], (float)vertex[2]);
	}

#if _DEBUG
	Timer::end(start, "Found (" + std::to_string(vertexCount) + ") vertices: ");
#endif
}

void FBXReader::readFBXTriangles(FbxMesh* mesh, MeshObject* outMesh)
{
	std::cout << "[MODELMAKER] Converting..." << std::endl;
	MeshStriper striper;
	striper.striper(mesh, outMesh);
}

void FBXReader::readFBXUVs(FbxMesh* mesh, MeshObject* outMesh)
{
#if _DEBUG
	auto start = Timer::begin();
#endif

	// uv coords
	FbxLayerElementArrayTemplate<FbxVector2>* uvArray = &mesh->GetElementUV()->GetDirectArray();
	outMesh->uvs.resize((size_t)uvArray->GetCount() * 2);
	float* uvs = outMesh->uvs.data();
	for (int i = 0; i < uvArray->GetCount(); i++) {
		int uvIndex = i * 2;
		FbxDouble* uvcoord = uvArray->GetAt(i).mData;
		uvs[uvIndex] = (float)uvcoord[0];
		uvs[uvIndex + 1] = (float)uvcoord[1];
	}

	// uv indices
	outMesh->uvIndexes.resize((size_t)mesh->GetPolygonCount() * 3);
	int* uvIndices = outMesh->uvIndexes.data();
	for (int i = 0; i < mesh->GetPolygonCount(); ++i) {
		int startIndex = i * 3;
		int uv0 = mesh->GetTextureUVIndex(i, 0, FbxLayerElement::eTextureDiffuse);
		int uv1 = mesh->GetTextureUVIndex(i, 1, FbxLayerElement::eTextureDiffuse);
		int uv2 = mesh->GetTextureUVIndex(i, 2, FbxLayerElement::eTextureDiffuse);
		uvIndices[startIndex + 0] = uv0;
		uvIndices[startIndex + 1] = uv1;
		uvIndices[startIndex + 2] = uv2;
	}
	
#if _DEBUG
	Timer::end(start, "Found (" + std::to_string(outMesh->uvs.size()) + ") uv's: ");
#endif
}
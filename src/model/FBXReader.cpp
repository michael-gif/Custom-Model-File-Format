#include <iostream>
#include <model/FBXReader.h>
#include <model/MeshObject.h>
#include <fbxsdk.h>
#include <util/Timer.hpp>
#include <string>
#include <chrono>
#include <striper/TriangleStripGenerator.h>
#include <numeric>

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
    //readFBXTriangles(mesh, outMesh);
    readTris(mesh, outMesh);
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

void FBXReader::readTris(FbxMesh* mesh, MeshObject* outMesh) {
    std::cout << "[MODELMAKER] Converting..." << std::endl;
    auto start = Timer::begin();
    int polygonCount = mesh->GetPolygonCount();
    int* vertices = mesh->GetPolygonVertices();
    uint32_t edge1, edge2, edge3;
    int vertex1, vertex2, vertex3;
    for (int i = 0; i < polygonCount * 3; i += 3) {
        vertex1 = vertices[i];
        vertex2 = vertices[i + 1];
        vertex3 = vertices[i + 2];
        if (vertex1 < vertex2)
            edge1 = (vertex1 << 16) | vertex2;
        else
            edge1 = (vertex2 << 16) | vertex1;

        if (vertex2 < vertex3)
            edge2 = (vertex2 << 16) | vertex3;
        else
            edge2 = (vertex3 << 16) | vertex2;

        if (vertex3 < vertex1)
            edge3 = (vertex3 << 16) | vertex1;
        else
            edge3 = (vertex1 << 16) | vertex3;
        outMesh->edges.emplace_back(edge1);
        outMesh->edges.emplace_back(edge2);
        outMesh->edges.emplace_back(edge3);
    }
    Timer::end(start, "Read (" + std::to_string(polygonCount) + ") triangles: ");
    striperNew(mesh, outMesh);
}

/// <summary>
/// Loop through the polygons and generate triangle strips, uv strips and a normal list
/// </summary>
/// <param name="mesh"></param>
/// <param name="outTriangles"></param>
void FBXReader::readFBXTriangles(FbxMesh* mesh, MeshObject* outMesh)
{
#if _DEBUG
    auto start = Timer::begin();
    int polygonCount = mesh->GetPolygonCount();
    Timer::end(start, "Read (" + std::to_string(polygonCount) + ") triangles: ");
#endif
    std::cout << "[MODELMAKER] Converting..." << std::endl;
    striper(mesh, outMesh);
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
    int* vertices = mesh->GetPolygonVertices();
    for (int i = 0; i < mesh->GetPolygonCount(); ++i) {
        mesh->GetPolygonVertexUV(i, 0, uvMapName, uvCoord, unmapped);
        outMesh->uvs.emplace_back(uvCoord[0]);
        outMesh->uvs.emplace_back(uvCoord[1]);
        mesh->GetPolygonVertexUV(i, 1, uvMapName, uvCoord, unmapped);
        outMesh->uvs.emplace_back(uvCoord[0]);
        outMesh->uvs.emplace_back(uvCoord[1]);
        mesh->GetPolygonVertexUV(i, 2, uvMapName, uvCoord, unmapped);
        outMesh->uvs.emplace_back(uvCoord[0]);
        outMesh->uvs.emplace_back(uvCoord[1]);
    }
#if _DEBUG
    Timer::end(start, "Read (" + std::to_string(outMesh->uvs.size()) + ") uv's: ");
#endif
}
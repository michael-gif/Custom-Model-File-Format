#include <iostream>
#include <FBXReader.h>
#include <MeshObject.h>
#include <fbxsdk.h>
#include <Timer.hpp>
#include <string>

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
    auto readStart = Timer::begin();
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

    readFBXVertices(mesh, outMesh);
    readFBXTriangles(mesh, outMesh);

    scene->Destroy();
    ioSettings->Destroy();
    manager->Destroy();

    std::flush(std::cout);
    Timer::end(readStart, "[FBX] Read model: ");
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
/// Compare 2 triangles, (x, y, z) and (a, b, c). If both triangles share 2 indices then they share an edge, and are considered adjacent.
/// Return the index that the second triangle doesn't share. This will be used as the next vertex in the triangle strip.
/// </summary>
/// <param name="x"></param>
/// <param name="y"></param>
/// <param name="z"></param>
/// <param name="a"></param>
/// <param name="b"></param>
/// <param name="c"></param>
/// <returns></returns>
int isAdjacentFront(int x, int y, int z, int a, int b, int c) {

    int numAdjacent = 0;
    bool b1 = false;
    bool b2 = false;
    bool b3 = false;
    if (y == a) {
        numAdjacent++;
        b1 = true;
    }
    else if (y == b) {
        numAdjacent++;
        b2 = true;
    }
    else if (y == c) {
        numAdjacent++;
        b3 = true;
    }

    /*
    * Check the flags first to avoid doing unecessary equality checks.
    * Example: If the y equals the a, then we don't need to check if the z equals the a, so skip it.
    */
    if (!b1 && z == a) {
        numAdjacent++;
        b1 = true;
    }
    else if (!b2 && z == b) {
        numAdjacent++;
        b2 = true;
    }
    else if (!b3 && z == c) {
        numAdjacent++;
        b3 = true;
    }
    if (numAdjacent == 2) {
        // when 2 indices are equal, the flag for the second index is set to true.
        // if 2 pair of indices are found, then the triangles share an edge and are therefore adjacent.
        // so return the flag that isn't true, and return the corresponding index from the second triangle.
        if (!b1) return a;
        if (!b2) return b;
        if (!b3) return c;
    }

    return 0;
}

/// <summary>
/// Same thing as isAdjacentFront, except we check the first two indices of against the second triangle instead of the last 2.
/// Example: for triangles (x, y, z) and (a, b, c), instead of doing checks for y and z against (a, b, c), do checks for x and y against (a, b, c)
/// Return the index that the second triangle doesn't share. This will be used as the next vertex in the triangle strip.
/// </summary>
/// <param name="x"></param>
/// <param name="y"></param>
/// <param name="z"></param>
/// <param name="a"></param>
/// <param name="b"></param>
/// <param name="c"></param>
/// <returns></returns>
int isAdjacentBehind(int x, int y, int z, int a, int b, int c) {

    int numAdjacent = 0;
    bool b1 = false;
    bool b2 = false;
    bool b3 = false;
    if (x == a) {
        numAdjacent++;
        b1 = true;
    }
    else if (x == b) {
        numAdjacent++;
        b2 = true;
    }
    else if (x == c) {
        numAdjacent++;
        b3 = true;
    }

    if (!b1 && y == a) {
        numAdjacent++;
        b1 = true;
    }
    else if (!b2 && y == b) {
        numAdjacent++;
        b2 = true;
    }
    else if (!b3 && y == c) {
        numAdjacent++;
        b3 = true;
    }
    if (numAdjacent == 2) {
        if (!b1) return a;
        if (!b2) return b;
        if (!b3) return c;
    }

    return 0;
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
#endif
    int polygonCount = mesh->GetPolygonCount();
    std::vector<int> indices;
    for (int i = 0; i < mesh->GetPolygonCount(); ++i) {
        indices.emplace_back(i);
    }
    std::vector<int> singleTriStrip;
    std::vector<float> singleUVStrip;
    int sizeondisk = 0;
    FbxStringList uvSetNames;
    mesh->GetUVSetNames(uvSetNames);
    FbxVector2 uvCoord;
    bool unmapped;

    while (true) {
        int front1 = mesh->GetPolygonVertex(indices[0], 0);
        int front2 = mesh->GetPolygonVertex(indices[0], 1);
        int front3 = mesh->GetPolygonVertex(indices[0], 2);
        int back1 = front1;
        int back2 = front2;
        int back3 = front3;
        singleTriStrip.emplace_back(front1);
        singleTriStrip.emplace_back(front2);
        singleTriStrip.emplace_back(front3);
        mesh->GetPolygonVertexUV(indices[0], 0, uvSetNames[0], uvCoord, unmapped);
        singleUVStrip.emplace_back(uvCoord[0]);
        singleUVStrip.emplace_back(uvCoord[1]);
        mesh->GetPolygonVertexUV(indices[0], 1, uvSetNames[0], uvCoord, unmapped);
        singleUVStrip.emplace_back(uvCoord[0]);
        singleUVStrip.emplace_back(uvCoord[1]);
        mesh->GetPolygonVertexUV(indices[0], 2, uvSetNames[0], uvCoord, unmapped);
        singleUVStrip.emplace_back(uvCoord[0]);
        singleUVStrip.emplace_back(uvCoord[1]);
        while (true) {
            int adjacentTris = 0;
            for (int i = 1; i < indices.size(); ++i) {
                int polygonIndex = indices[i];
                int startIndex = mesh->GetPolygonVertexIndex(polygonIndex);
                int* vertices = mesh->GetPolygonVertices();
                int p1 = vertices[startIndex + 0];
                int p2 = vertices[startIndex + 1];
                int p3 = vertices[startIndex + 2];

                int adjacentFront = isAdjacentFront(front1, front2, front3, p1, p2, p3);
                if (adjacentFront) {
                    front1 = front2;
                    front2 = front3;
                    front3 = adjacentFront;
                    singleTriStrip.emplace_back(adjacentFront);
                    mesh->GetPolygonVertexUV(polygonIndex, 0, uvSetNames[0], uvCoord, unmapped);
                    singleUVStrip.emplace_back(uvCoord[0]);
                    singleUVStrip.emplace_back(uvCoord[1]);
                    indices[i] = -1; // mark triangle for erasure
                    adjacentTris++;
                    continue;
                }
                int adjacentBehind = isAdjacentBehind(back1, back2, back3, p1, p2, p3);
                if (adjacentBehind) {
                    back3 = back2;
                    back2 = back1;
                    back1 = adjacentBehind;
                    singleTriStrip.insert(singleTriStrip.begin(), adjacentBehind);
                    mesh->GetPolygonVertexUV(polygonIndex, 0, uvSetNames[0], uvCoord, unmapped);
                    singleUVStrip.insert(singleUVStrip.begin(), uvCoord[1]);
                    singleUVStrip.insert(singleUVStrip.begin(), uvCoord[0]);
                    indices[i] = -1; // mark triangle for erasure
                    adjacentTris++;
                }
            }
            if (!adjacentTris) break; // if no adjacent triangles were found, the strip has ended.

            // erase any triangles that were put into a strip
            int indicesSize = indices.size();
            for (int i = 0; i < indicesSize; ++i) {
                if (indices[i] == -1) {
                    indices.erase(indices.begin() + i);
                    i--;
                }
                indicesSize = indices.size();
            }
        }
        indices.erase(indices.begin()); // the first triangle was never removed during the loop, so remove it now.
#if _DEBUG
        sizeondisk += 2 + (singleTriStrip.size() * 2); // statistics
        sizeondisk += 2 + (singleUVStrip.size() * 2);
#endif
        outMesh->triangleStrips.emplace_back(singleTriStrip);
        outMesh->uvStrips.emplace_back(singleUVStrip);
        singleTriStrip.clear();
        singleUVStrip.clear();
        if (indices.empty()) break; // no more triangles mean we have created all the strips needed.
    }
#if _DEBUG
    std::cout << "Triangle Strips: " << outMesh->triangleStrips.size() << "\n";
    std::cout << "UV Strips: " << outMesh->uvStrips.size() << "\n";
    std::cout << "Size on disk before: " << polygonCount * 18 << " bytes\n";
    std::cout << "Size on disk after: " << sizeondisk << " bytes\n";
    std::cout << "Saving: " << (1 - ((float)sizeondisk / (float)(polygonCount * 18))) * 100 << "%" << std::endl;
    Timer::end(start, "Read (" + std::to_string(polygonCount) + ") triangles and uv's: ");
#endif
}
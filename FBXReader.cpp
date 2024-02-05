#include <iostream>
#include <FBXReader.h>
#include <MeshObject.h>
#include <fbxsdk.h>
#include <Timer.hpp>
#include <string>
#include <chrono>

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
    readFBXUVs(mesh, outMesh);

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
int isAdjacentFront(int y, int z, int a, int b, int c) {

    bool b1 = false;
    bool b2 = false;
    bool b3 = false;
    if (y == a) {
        b1 = true;
    }
    else if (y == b) {
        b2 = true;
    }
    else if (y == c) {
        b3 = true;
    }
    else {
        // If the y element isn't equal to a, b or c, then it is guaranteed that triangle a,b,c is not adjacent to x,y,z
        return 0;
    }

    /*
    * Check the flags first to avoid doing unecessary equality checks.
    * Example: If the y equals the a, then we don't need to check if the z equals the a, so skip it.
    */
    if (!b1 && z == a) {
        b1 = true;
    }
    else if (!b2 && z == b) {
        b2 = true;
    }
    else if (!b3 && z == c) {
        b3 = true;
    }
    else {
        // If the z element isn't equal to a, b or c, then it is guaranteed that triangle a,b,c is not adjacent to x,y,z
        return 0;
    }
    // when 2 indices are equal, the flag for the second index is set to true.
    // if 2 pair of indices are found, then the triangles share an edge and are therefore adjacent.
    // so return the flag that isn't true, and return the corresponding index from the second triangle.
    if (!b1) return a;
    if (!b2) return b;
    if (!b3) return c;

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
int isAdjacentBehind(int x, int y, int a, int b, int c) {

    bool b1 = false;
    bool b2 = false;
    bool b3 = false;
    if (x == a) {
        b1 = true;
    }
    else if (x == b) {
        b2 = true;
    }
    else if (x == c) {
        b3 = true;
    }
    else {
        // If the x element isn't equal to a, b or c, then it is guaranteed that triangle a,b,c is not adjacent to x,y,z
        return 0;
    }

    if (!b1 && y == a) {
        b1 = true;
    }
    else if (!b2 && y == b) {
        b2 = true;
    }
    else if (!b3 && y == c) {
        b3 = true;
    }
    else {
        // If the y element isn't equal to a, b or c, then it is guaranteed that triangle a,b,c is not adjacent to x,y,z
        return 0;
    }
    if (!b1) return a;
    if (!b2) return b;
    if (!b3) return c;

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
    std::vector<int> triangles;
    for (int i = 0; i < mesh->GetPolygonCount(); ++i) {
        triangles.emplace_back(i);
    }
    std::vector<int> singleTriStrip;
    int sizeondisk = 0;
    bool unmapped;
    int* vertices = mesh->GetPolygonVertices();

    std::cout << "0";
    float previousCompletion = 0;
    float completion = 0;
    while (true) {
        int firstTriangleIndex = triangles[0];
        int firstStartIndex = mesh->GetPolygonVertexIndex(firstTriangleIndex);
        int frontX = vertices[firstStartIndex];
        int frontY = vertices[firstStartIndex + 1];
        int frontZ = vertices[firstStartIndex + 2];
        int backX = frontX;
        int backY = frontY;
        singleTriStrip.emplace_back(frontX);
        singleTriStrip.emplace_back(frontY);
        singleTriStrip.emplace_back(frontZ);

        int numTriangles = triangles.size();
        while (true) {
            int adjacentTris = 0;
            for (int i = 1; i < numTriangles; ++i) {
                int polygonIndex = triangles[i];
                int startIndex = mesh->GetPolygonVertexIndex(polygonIndex);
                int p1 = vertices[startIndex + 0];
                int p2 = vertices[startIndex + 1];
                int p3 = vertices[startIndex + 2];

                int adjacentFront = isAdjacentFront(frontY, frontZ, p1, p2, p3);
                if (adjacentFront) {
                    frontY = frontZ;
                    frontZ = adjacentFront;
                    singleTriStrip.emplace_back(adjacentFront);
                    ++adjacentTris;

                    // erase triangle index from vector.
                    triangles.erase(triangles.begin() + i);
                    --i; // i gets incremented every iteration, so shift it back one to prevent the next element being skipped
                    --numTriangles; // loop end is dynamic, so adjust it accordingly
                    continue;
                }
                int adjacentBehind = isAdjacentBehind(backX, backY, p1, p2, p3);
                if (adjacentBehind) {
                    backY = backX;
                    backX = adjacentBehind;
                    singleTriStrip.insert(singleTriStrip.begin(), adjacentBehind);
                    ++adjacentTris;

                    // erase triangle index from vector.
                    triangles.erase(triangles.begin() + i);
                    --i; // i gets incremented every iteration, so shift it back one to prevent the next element being skipped
                    --numTriangles; // loop end is dynamic, so adjust it accordingly
                }
            }
            if (!adjacentTris) break; // if no adjacent triangles were found, the strip has ended.
        }
        triangles.erase(triangles.begin()); // the first triangle was never removed during the loop, so remove it now.
#if _DEBUG
        sizeondisk += 2 + (singleTriStrip.size() * 2); // statistics
#endif
        outMesh->triangleStrips.emplace_back(singleTriStrip);
        singleTriStrip.clear();

        // progress bar
        completion = (1 - ((float)triangles.size() / (float)polygonCount)) * 40; // value between 1 and 40
        int rounded = static_cast<int>(completion);
        int diff = rounded - previousCompletion;
        if (diff) {
            if (rounded % 4 == 0)
                std::cout << (previousCompletion + 1) / 4;
            else
                std::cout << ".";
            previousCompletion = rounded;
        }

        if (triangles.empty()) break; // no more triangles mean we have created all the strips needed.
    }
    std::cout << std::endl;
#if _DEBUG
    std::cout << "Triangle strips: " << outMesh->triangleStrips.size() << "\n";
    std::cout << "Size on disk before: " << polygonCount * 18 << " bytes\n";
    std::cout << "Size on disk after: " << sizeondisk << " bytes\n";
    std::cout << "Saving: " << (1 - ((float)sizeondisk / (float)(polygonCount * 18))) * 100 << "%" << std::endl;
    Timer::end(start, "Read (" + std::to_string(polygonCount) + ") triangles: ");
#endif
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
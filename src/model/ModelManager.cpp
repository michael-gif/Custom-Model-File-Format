#include <iostream>
#include <string>
#include <cstring>
#include <vector>
#include <fstream>
#include <fbxsdk.h>
#include <model/ModelManager.h>
#include <model/MeshObject.h>
#include <model/FBXReader.h>
#include <util/Timer.hpp>

void ModelManager::compare(MeshObject* meshA, MeshObject* meshB) {
    //compare vertices
    int meshAVerts = (int)meshA->vertices.size();
    int meshBVerts = (int)meshB->vertices.size();
    std::cout << "Vertices: " << meshAVerts << "/" << meshBVerts << ", ";
    int numCorrect = 0;
    for (int i = 0; i < meshBVerts; ++i) {
        MeshObject::Vertex a = meshA->vertices[i];
        MeshObject::Vertex b = meshB->vertices[i];
        if ((a.x == b.x) && (a.y == b.y) && (a.z == b.z)) numCorrect++;
    }
    std::cout << ((float)numCorrect / (float)meshBVerts) * 100 << "% accurate" << std::endl;

    // compare triangle strips
    int meshATriStrips = (int)meshA->triangleStrips.size();
    int meshBTriStrips = (int)meshB->triangleStrips.size();
    std::cout << "Triangle strips: " << meshATriStrips << "/" << meshBTriStrips << ", ";
    if (meshATriStrips != meshBTriStrips) return;
    int numIndices = 0;
    numCorrect = 0;
    for (int i = 0; i < meshBTriStrips; i++) {
        for (int j = 0; j < meshB->triangleStrips[i].size(); j++) {
            numIndices++;
            if (meshA->triangleStrips[i][j] == meshB->triangleStrips[i][j]) numCorrect++;
        }
    }
    std::cout << ((float)numCorrect / (float)numIndices) * 100 << "% accurate" << std::endl;

    // compare uv strips
    int uvsA = (int)meshA->uvs.size();
    int uvsB = (int)meshB->uvs.size();
    std::cout << "UV coords: " << uvsA << "/" << uvsB << ", ";
    numCorrect = 0;
    for (int i = 0; i < uvsB; i++) {
        float b = (float)static_cast<uint16_t>(meshB->uvs[i] * 10000) / 10000;
        if (meshA->uvs[i] == b) numCorrect++;
    }
    std::cout << ((float)numCorrect / (float)uvsB) * 100 << "% accurate" << std::endl;

    // compare vertex normals
    numCorrect = 0;
    for (int i = 0; i < meshB->vertices.size(); ++i) {
        MeshObject::Normal normalA = meshA->vertices[i].normal;
        MeshObject::Normal normalB = meshB->vertices[i].normal;
        if (normalA.x == normalB.x && normalA.y == normalB.y && normalA.z == normalB.z) numCorrect++;
    }
    std::cout << "Normals: " << ((float)numCorrect / (float)meshA->vertices.size()) * 100 << "% accurate" << std::endl;
}

/// <summary>
/// Read model file
/// </summary>
/// <param name="path"></param>
/// <returns>Pointer to loaded mesh object</returns>
void ModelManager::readModel(const char* path, MeshObject* outMesh)
{
    auto start = Timer::begin();
#if _DEBUG
    Timer::end(start, "Created empty mesh: ");
#endif
    std::ifstream file(path, std::ios::binary);
    readVertices(file, outMesh);
    readTriangleStrips(file, outMesh);
    readUVs(file, outMesh);
    readVertexNormals(file, outMesh);
    file.close();
    Timer::end(start, "[MODELMAKER] Read model: ");
    std::flush(std::cout);
}

/// <summary>
/// File is read in chunks to reduce overhead from file::read. Maximum chunksize seems to be 64, meaning 64 vertices are read at a time.
/// The remaining vertices are read 1 by 1 at the end.
/// Each vertex is 3 floats, so 12 bytes a vertex.
/// </summary>
/// <param name="file"></param>
/// <returns></returns>
void ModelManager::readVertices(std::ifstream& file, MeshObject* mesh)
{
#if _DEBUG
    auto start = Timer::begin();
#endif
    char metadataBuffer[2];
    file.read(metadataBuffer, sizeof(metadataBuffer));
    uint16_t numVertices = *reinterpret_cast<uint16_t*>(&metadataBuffer);
    mesh->vertices.resize(numVertices);
    MeshObject::Vertex* vertices = mesh->vertices.data();
    int numBytes = 12 * numVertices;
    std::vector<char> vertexBuffer(numBytes);
    file.read(vertexBuffer.data(), numBytes);
    float* vertexData = reinterpret_cast<float*>(vertexBuffer.data());
    for (int i = 0; i < numVertices; ++i) {
        int startIndex = i * 3;
        vertices[i].setPos(vertexData[startIndex], vertexData[startIndex + 1], vertexData[startIndex + 2]);
    }
#if _DEBUG
    Timer::end(start, "Read (" + std::to_string(mesh->vertices.size()) + ") vertices: ");
#endif
}

/// <summary>
/// Read triangle strips in chunks to reduce overhead from file::read. Maximum chunk size seems to be 64, meaning 64 vertices are read at a time.
/// Each vertex index is 2 bytes.
/// </summary>
/// <param name="file"></param>
/// <returns></returns>
void ModelManager::readTriangleStrips(std::ifstream& file, MeshObject* mesh)
{
#if _DEBUG
    auto start = Timer::begin();
#endif
    char metadataBuffer[2];
    char stripSizeBuffer[2];
    std::vector<char> vertexIndexBuffer;
    file.read(metadataBuffer, sizeof(metadataBuffer));
    uint16_t numTriStrips = *reinterpret_cast<uint16_t*>(&metadataBuffer);
    mesh->triangleStrips.resize(numTriStrips);
    std::vector<uint16_t>* triStrips = mesh->triangleStrips.data();
    for (int i = 0; i < numTriStrips; ++i) {
        file.read(stripSizeBuffer, sizeof(stripSizeBuffer));
        uint16_t stripSize = *reinterpret_cast<uint16_t*>(&stripSizeBuffer);
        int numBytes = 2 * stripSize;
        triStrips[i].resize(stripSize);
        uint16_t* strip = triStrips[i].data();
        vertexIndexBuffer.resize(numBytes);
        file.read(vertexIndexBuffer.data(), numBytes);
        std::memcpy(strip, vertexIndexBuffer.data(), numBytes);

    }
#if _DEBUG
    Timer::end(start, "Read (" + std::to_string(mesh->triangleStrips.size()) + ") triangle strips: ");
#endif
}

/// <summary>
/// Read UV coords in chunks to reduce overhead from file::read. Maximum chunk size seems to be 256;
/// Remaing uvs are read 1 by 1 at the end.
/// Each uv is 2 bytes.
/// </summary>
/// <param name="file"></param>
/// <param name="outCoords"></param>
void ModelManager::readUVs(std::ifstream& file, MeshObject* mesh)
{
#if _DEBUG
    auto start = Timer::begin();
#endif
    char metadataBuffer[2];
    file.read(metadataBuffer, sizeof(metadataBuffer));
    uint16_t numUVs = *reinterpret_cast<uint16_t*>(&metadataBuffer);
    int numBytes = 2 * numUVs;
    std::vector<char> buffer(numBytes);
    file.read(buffer.data(), numBytes);
    uint16_t* uvsPtr = reinterpret_cast<uint16_t*>(buffer.data());
    mesh->uvs.resize(numUVs);
    float* meshUvsPtr = mesh->uvs.data();
    for (int i = 0; i < numUVs; ++i) {
        meshUvsPtr[i] = (float)uvsPtr[i] / 10000;
    }

#if _DEBUG
    Timer::end(start, "Read (" + std::to_string(mesh->uvs.size()) + ") uvs: ");
#endif
}

/// <summary>
/// Read vertex normals in chunks to reduce overhead from file::read. Maximum chunk size seems to be 64, meaning 64 normals are read at a time.
/// Remaining normls are read 1 by 1 at the end.
/// Each normal is 3 floats, so 12 bytes a normal.
/// </summary>
/// <param name="file"></param>
/// <param name="mesh"></param>
void ModelManager::readVertexNormals(std::ifstream& file, MeshObject* mesh)
{
#if _DEBUG
    auto start = Timer::begin();
#endif
    char metadataBuffer[2];
    file.read(metadataBuffer, sizeof(metadataBuffer));
    uint16_t numVertexNormals = *reinterpret_cast<uint16_t*>(&metadataBuffer);
    int numBytes = 12 * numVertexNormals;
    std::vector<char> buffer(numBytes);
    file.read(buffer.data(), numBytes);
    MeshObject::Vertex* vertices = mesh->vertices.data();
    float* normals = reinterpret_cast<float*>(buffer.data());
    for (int i = 0; i < numVertexNormals; ++i) {
        int startIndex = i * 3;
        vertices[i].setNormal(normals[startIndex], normals[startIndex + 1], normals[startIndex + 2]);
    }
#if _DEBUG
    Timer::end(start, "Read (" + std::to_string(numVertexNormals) + ") vertex normals: ");
#endif
}

/// <summary>
/// Write MeshObject to .m file
/// </summary>
/// <param name="mesh"></param>
/// <param name="filename"></param>
void ModelManager::writeToDisk(MeshObject* mesh, std::string filename)
{
    auto start = Timer::begin();
    std::ofstream modelFile(filename, std::ios::out | std::ios::binary);
    writeVertices(mesh, modelFile);
    writeTriangleStrips(mesh, modelFile);
    writeUVs(mesh, modelFile);
    writeVertexNormals(mesh, modelFile);
    Timer::end(start, "[MODELMAKER] Wrote model to disk (" + std::to_string(mesh->sizeondisk) + " bytes): ");
}

/// <summary>
/// Each vertex is represented as 3 floats of 4 bytes each, for the x, y, and z, so 12 bytes per vertex.
/// The vertex bytes are stored directly next to each other, with a vertex count at the start.
/// Each vertex has a UV coordinate. A coord is 2 floats, 4 bytes each, so 8 bytes per uv coord.
/// UV coordinate bytes are stored after vertex bytes.
/// 
/// There is a maximum of 65,536 vertices, since the triangles use 2 bytes per vertex index.
/// If you want more vertices, the triangles will have to use 3 or 4 bytes per index, which will increase file size.
/// </summary>
/// <param name="mesh"></param>
/// <param name="file"></param>
void ModelManager::writeVertices(MeshObject* mesh, std::ofstream& file)
{
#if _DEBUG
    auto start = Timer::begin();
#endif
    int numVertices = (int)mesh->vertices.size();
    file.write(reinterpret_cast<const char*>(&numVertices), 2);
    std::vector<float> values(numVertices * 3);
    float* ptr = values.data();
    MeshObject::Vertex* vertices = mesh->vertices.data();
    for (int i = 0; i < numVertices; ++i) {
        int startIndex = i * 3;
        MeshObject::Vertex v = vertices[i];
        ptr[startIndex + 0] = v.x;
        ptr[startIndex + 1] = v.y;
        ptr[startIndex + 2] = v.z;
    }
    file.write(reinterpret_cast<const char*>(ptr), 4 * values.size());
    int numBytes = 2 + (numVertices * 12);
    mesh->sizeondisk += numBytes;
#if _DEBUG
    Timer::end(start, "Wrote (" + std::to_string(mesh->vertices.size()) + ") vertices (" + std::to_string(numBytes) + " bytes): ");
#endif
}

/// <summary>
/// Write triangle strips
/// </summary>
/// <param name="mesh"></param>
/// <param name="file"></param>
void ModelManager::writeTriangleStrips(MeshObject* mesh, std::ofstream& file)
{
#if _DEBUG
    auto start = Timer::begin();
#endif
    int numTriStrips = (int)mesh->triangleStrips.size();
    file.write(reinterpret_cast<const char*>(&numTriStrips), 2);
    int numBytes = 0;
    for (int i = 0; i < numTriStrips; ++i) {
        std::vector<uint16_t> strip = mesh->triangleStrips[i];
        uint16_t stripSize = (uint16_t)strip.size();
        int numStripBytes = 2 * stripSize;
        numBytes += 2 + (numStripBytes);
        file.write(reinterpret_cast<const char*>(&stripSize), 2);
        file.write(reinterpret_cast<const char*>(strip.data()), numStripBytes);
    }
    mesh->sizeondisk += 2 + numBytes;
#if _DEBUG
    Timer::end(start, "Wrote (" + std::to_string(mesh->triangleStrips.size()) + ") triangle strips (" + std::to_string(numBytes) + " bytes): ");
#endif
}

/// <summary>
/// Write UV coord strips
/// </summary>
/// <param name="mesh"></param>
/// <param name="file"></param>
void ModelManager::writeUVs(MeshObject* mesh, std::ofstream& file)
{
#if _DEBUG
    auto start = Timer::begin();
#endif
    int numUVs = (int)mesh->uvs.size();
    float* uvs = mesh->uvs.data();
    std::vector<uint16_t> writableUvs(numUVs);
    uint16_t* writableUvsPtr = writableUvs.data();
    file.write(reinterpret_cast<const char*>(&numUVs), 2);
    for (int i = 0; i < numUVs; ++i) {
        writableUvsPtr[i] = static_cast<uint16_t>(uvs[i] * 10000);
    }
    int numBytes = 2 * numUVs;
    file.write(reinterpret_cast<const char*>(writableUvsPtr), numBytes);
    mesh->sizeondisk += 2 + (numBytes);
#if _DEBUG
    Timer::end(start, "Wrote (" + std::to_string(numUVs) + ") uv coords (" + std::to_string(2 + (numBytes)) + " bytes): ");
#endif
}

/// <summary>
/// Write vertex normals. 12 bytes a vertex.
/// </summary>
/// <param name="mesh"></param>
/// <param name="file"></param>
void ModelManager::writeVertexNormals(MeshObject* mesh, std::ofstream& file)
{
#if _DEBUG
    auto start = Timer::begin();
#endif
    int numVertexNormals = (int)mesh->vertices.size();
    file.write(reinterpret_cast<const char*>(&numVertexNormals), 2);
    int numBytes = numVertexNormals * 12;
    mesh->sizeondisk += numBytes;
    MeshObject::Vertex* vertices = mesh->vertices.data();
    std::vector<float> normals(numVertexNormals * 3);
    float* normalsPtr = normals.data();
    for (int i = 0; i < numVertexNormals; ++i) {
        int startIndex = i * 3;
        MeshObject::Normal normal = vertices[i].normal;
        normalsPtr[startIndex + 0] = normal.x;
        normalsPtr[startIndex + 1] = normal.y;
        normalsPtr[startIndex + 2] = normal.z;
    }
    file.write(reinterpret_cast<const char*>(normals.data()), 4 * normals.size());
#if _DEBUG
    Timer::end(start, "Wrote (" + std::to_string(numVertexNormals) + ") vertex normals (" + std::to_string(numBytes) + " bytes): ");
#endif
}
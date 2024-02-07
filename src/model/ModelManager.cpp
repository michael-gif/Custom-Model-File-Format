#include <string>
#include <vector>
#include <fstream>
#include <model/ModelManager.h>
#include <model/MeshObject.h>
#include <chrono>
#include <iostream>
#include <util/Timer.hpp>
#include <fbxsdk.h>
#include <iomanip>
#include <model/FBXReader.h>

void ModelManager::compare(MeshObject* meshA, MeshObject* meshB) {
    // compare triangle strips
    int meshATriStrips = meshA->triangleStrips.size();
    int meshBTriStrips = meshB->triangleStrips.size();
    std::cout << meshATriStrips << "/" << meshBTriStrips << " triangle strips\n";
    if (meshATriStrips != meshBTriStrips) return;
    int numIndices = 0;
    int numCorrect = 0;
    for (int i = 0; i < meshBTriStrips; i++) {
        std::cout << "[STRIP-" << i << "] " << meshA->triangleStrips[i].size() << "/" << meshB->triangleStrips[i].size() << " indices\n";
    }
    for (int i = 0; i < meshBTriStrips; i++) {
        for (int j = 0; j < meshB->triangleStrips[i].size(); j++) {
            numIndices++;
            if (meshA->triangleStrips[i][j] == meshB->triangleStrips[i][j]) numCorrect++;
        }
    }
    std::cout << "Triangle strips are " << ((float)numCorrect / (float)numIndices) * 100 << "% accurate" << std::endl;

    // compare uv strips
    int uvsA = meshA->uvs.size();
    int uvsB = meshB->uvs.size();
    std::cout << uvsA << "/" << uvsB << " uv strips\n";
    if (uvsA != uvsB) return;
    numCorrect = 0;
    for (int i = 0; i < uvsA; i++) {
        float b = (float)static_cast<uint16_t>(meshB->uvs[i] * 10000) / 10000;
        if (meshA->uvs[i] == b) numCorrect++;
    }
    std::cout << "UV coords are " << ((float)numCorrect / (float)uvsA) * 100 << "% accurate" << std::endl;
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
    readNormals(file, outMesh);
    file.close();
    Timer::end(start, "[MODELMAKER] Read model: ");
    std::flush(std::cout);
}

/// <summary>
/// Reads file 20 bytes at a time, converting them into 3 floats and generating vertices
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
    char buffer[12];
    for (int i = 0; i < numVertices; ++i) {
        file.read(buffer, sizeof(buffer));
        float x = *reinterpret_cast<float*>(&buffer);
        float y = *(reinterpret_cast<float*>(&buffer) + 1);
        float z = *(reinterpret_cast<float*>(&buffer) + 2);
        mesh->vertices.emplace_back(x, y, z);
    }
#if _DEBUG
    Timer::end(start, "Read (" + std::to_string(mesh->vertices.size()) + ") vertices: ");
#endif
}

/// <summary>
/// Read triangle strips
/// </summary>
/// <param name="file"></param>
/// <returns></returns>
void ModelManager::readTriangleStrips(std::ifstream& file, MeshObject* mesh)
{
#if _DEBUG
    auto start = Timer::begin();
#endif
    char metadataBuffer[2];
    file.read(metadataBuffer, sizeof(metadataBuffer));
    uint16_t numTriStrips = *reinterpret_cast<uint16_t*>(&metadataBuffer);
    for (int i = 0; i < numTriStrips; ++i) {
        mesh->triangleStrips.emplace_back();
        char stripSizeBuffer[2];
        file.read(stripSizeBuffer, sizeof(stripSizeBuffer));
        uint16_t stripSize = *reinterpret_cast<uint16_t*>(&stripSizeBuffer);
        for (int j = 0; j < stripSize; ++j) {
            char vertexIndexBuffer[2];
            file.read(vertexIndexBuffer, 2);
            uint16_t vertexIndex = *reinterpret_cast<uint16_t*>(&vertexIndexBuffer);
            mesh->triangleStrips[i].emplace_back(vertexIndex);
        }
    }
#if _DEBUG
    Timer::end(start, "Read (" + std::to_string(mesh->triangleStrips.size()) + ") triangle strips: ");
#endif
}

/// <summary>
/// Read UV coordinate strips
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
    for (int i = 0; i < numUVs; ++i) {
        char uvCoordBuffer[2];
        file.read(uvCoordBuffer, 2);
        uint16_t uvcomponent = *reinterpret_cast<uint16_t*>(&uvCoordBuffer);
        mesh->uvs.emplace_back((float)uvcomponent / 10000);
    }
#if _DEBUG
    Timer::end(start, "Read (" + std::to_string(mesh->uvs.size()) + ") uvs: ");
#endif
}

/// <summary>
/// Read triangle strips
/// </summary>
/// <param name="file"></param>
/// <returns></returns>
void ModelManager::readNormals(std::ifstream& file, MeshObject* mesh)
{
#if _DEBUG
    auto start = Timer::begin();
#endif
    char metadataBuffer[2];
    file.read(metadataBuffer, sizeof(metadataBuffer));
    uint16_t numNormalStrips = *reinterpret_cast<uint16_t*>(&metadataBuffer);
    for (int i = 0; i < numNormalStrips; ++i) {
        mesh->normalStrips.emplace_back();
        char stripSizeBuffer[2];
        file.read(stripSizeBuffer, sizeof(stripSizeBuffer));
        uint16_t stripSize = *reinterpret_cast<uint16_t*>(&stripSizeBuffer);
        for (int j = 0; j < stripSize; ++j) {
            char normalBuffer[4];
            file.read(normalBuffer, 4);
            float normal = *reinterpret_cast<float*>(&normalBuffer);
            mesh->normalStrips[i].emplace_back(normal);
        }
    }
#if _DEBUG
    Timer::end(start, "Read (" + std::to_string(mesh->normalStrips.size()) + ") normal strips: ");
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
    writeNormals(mesh, modelFile);
    modelFile.close();
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
    int numVertices = mesh->vertices.size();
    file.write(reinterpret_cast<const char*>(&numVertices), 2);
    for (int i = 0; i < numVertices; ++i) {
        file.write(reinterpret_cast<const char*>(&mesh->vertices[i].x), 4);
        file.write(reinterpret_cast<const char*>(&mesh->vertices[i].y), 4);
        file.write(reinterpret_cast<const char*>(&mesh->vertices[i].z), 4);
    }
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
    int numTriStrips = mesh->triangleStrips.size();
    file.write(reinterpret_cast<const char*>(&numTriStrips), 2);
    int numBytes = 0;
    for (int i = 0; i < numTriStrips; ++i) {
        std::vector<uint16_t> strip = mesh->triangleStrips[i];
        uint16_t stripSize = (uint16_t)strip.size();
        //std::cout << stripSize << std::endl;
        numBytes += 2 + (stripSize * 2);
        file.write(reinterpret_cast<const char*>(&stripSize), 2);
        for (int j = 0; j < stripSize; ++j) {
            file.write(reinterpret_cast<const char*>(&strip[j]), 2);
        }
    }
    mesh->sizeondisk += numBytes;
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
    uint16_t numUVs = mesh->uvs.size();
    file.write(reinterpret_cast<const char*>(&numUVs), 2);
    for (int i = 0; i < numUVs; ++i) {
        uint16_t uvInt = static_cast<uint16_t>(mesh->uvs[i] * 10000);
        file.write(reinterpret_cast<const char*>(&uvInt), 2);
    }
    mesh->sizeondisk += numUVs * 2;
#if _DEBUG
    Timer::end(start, "Wrote (" + std::to_string(numUVs) + ") uv coords (" + std::to_string(numUVs * 2) + " bytes): ");
#endif
}

/// <summary>
/// Write normals
/// </summary>
/// <param name="mesh"></param>
/// <param name="file"></param>
void ModelManager::writeNormals(MeshObject* mesh, std::ofstream& file)
{
#if _DEBUG
    auto start = Timer::begin();
#endif
    int numNormalStrips = mesh->triangleStrips.size();
    file.write(reinterpret_cast<const char*>(&numNormalStrips), 2);
    int numBytes = 0;
    float normal = 6.9;
    for (int i = 0; i < numNormalStrips; ++i) {
        std::vector<uint16_t> strip = mesh->triangleStrips[i];
        int stripSize = strip.size() - 2;
        numBytes += 2 + (stripSize * 4);
        file.write(reinterpret_cast<const char*>(&stripSize), 2);
        for (int j = 0; j < stripSize; ++j) {
            file.write(reinterpret_cast<const char*>(&normal), 4);
        }
    }
    mesh->sizeondisk += numBytes;
#if _DEBUG
    Timer::end(start, "Wrote (" + std::to_string(mesh->triangleStrips.size()) + ") normal strips (" + std::to_string(numBytes) + " bytes): ");
#endif
}
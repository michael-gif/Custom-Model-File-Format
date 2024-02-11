#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <fbxsdk.h>
#include <model/ModelManager.h>
#include <model/MeshObject.h>
#include <model/FBXReader.h>
#include <util/Timer.hpp>

void ModelManager::compare(MeshObject* meshA, MeshObject* meshB) {
    //compare vertices
    int meshAVerts = meshA->vertices.size();
    int meshBVerts = meshB->vertices.size();
    std::cout << meshAVerts << "/" << meshBVerts << " vertices\n";
    int numCorrect = 0;
    for (int i = 0; i < meshBVerts; ++i) {
        MeshObject::Vertex a = meshA->vertices[i];
        MeshObject::Vertex b = meshB->vertices[i];
        if ((a.x == b.x) && (a.y == b.y) && (a.z == b.z)) numCorrect++;
    }
    std::cout << "Vertices are " << ((float)numCorrect / (float)meshBVerts) * 100 << "% accurate" << std::endl;

    // compare triangle strips
    int meshATriStrips = meshA->triangleStrips.size();
    int meshBTriStrips = meshB->triangleStrips.size();
    std::cout << meshATriStrips << "/" << meshBTriStrips << " triangle strips\n";
    if (meshATriStrips != meshBTriStrips) return;
    int numIndices = 0;
    numCorrect = 0;
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
    std::cout << uvsA << "/" << uvsB << " uvs\n";
    numCorrect = 0;
    for (int i = 0; i < uvsB; i++) {
        float b = (float)static_cast<uint16_t>(meshB->uvs[i] * 10000) / 10000;
        if (meshA->uvs[i] == b) numCorrect++;
    }
    std::cout << "UV coords are " << ((float)numCorrect / (float)uvsB) * 100 << "% accurate" << std::endl;

    // compare vertex normals
    numCorrect = 0;
    for (int i = 0; i < meshB->vertices.size(); ++i) {
        MeshObject::Normal normalA = meshA->vertices[i].normal;
        MeshObject::Normal normalB = meshB->vertices[i].normal;
        if (normalA.x == normalB.x && normalA.y == normalB.y && normalA.z == normalB.z) numCorrect++;
    }
    std::cout << "Normals are " << ((float)numCorrect / (float)meshA->vertices.size()) * 100 << "% accurate" << std::endl;
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

    int chunksize = 1;
    if (numVertices >= 64) chunksize = 64;
    else if (numVertices >= 32) chunksize = 32;
    else if (numVertices >= 16) chunksize = 16;
    else if (numVertices >= 8) chunksize = 8;
    else if (numVertices >= 4) chunksize = 4;

    std::vector<char> buffer(12 * chunksize);
    char remainderBuffer[12];
    int remainder = numVertices % chunksize;
    mesh->vertices.resize(numVertices);
    MeshObject::Vertex* vertices = mesh->vertices.data();
    for (int i = 0; i < numVertices / chunksize; ++i) {
        int startIndex = i * chunksize;
        file.read(buffer.data(), 12 * chunksize);
        float* vertex = reinterpret_cast<float*>(buffer.data());
        for (int j = 0; j < chunksize; ++j) {
            vertices[startIndex + j].setPos(vertex[j * 3], vertex[(j * 3) + 1], vertex[(j * 3) + 2]);
        }
    }
    int startIndex = numVertices - remainder;
    for (int i = 0; i < remainder; ++i) {
        file.read(remainderBuffer, sizeof(remainderBuffer));
        float* vertex = reinterpret_cast<float*>(&remainderBuffer);
        vertices[startIndex + i].setPos(vertex[0], vertex[1], vertex[2]);
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
    char remainderBuffer[2];
    std::vector<char> vertexIndexBuffer;
    file.read(metadataBuffer, sizeof(metadataBuffer));
    uint16_t numTriStrips = *reinterpret_cast<uint16_t*>(&metadataBuffer);
    int chunksize = 1;
    mesh->triangleStrips.resize(numTriStrips);
    std::vector<uint16_t>* triStrips = mesh->triangleStrips.data();
    for (int i = 0; i < numTriStrips; ++i) {
        file.read(stripSizeBuffer, sizeof(stripSizeBuffer));
        uint16_t stripSize = *reinterpret_cast<uint16_t*>(&stripSizeBuffer);

        if (stripSize >= 64) chunksize = 64;
        else if (stripSize >= 32) chunksize = 32;
        else if (stripSize >= 16) chunksize = 16;
        else if (stripSize >= 8) chunksize = 8;
        else if (stripSize >= 4) chunksize = 4;

        triStrips[i].resize(stripSize);
        uint16_t* strip = triStrips[i].data();
        vertexIndexBuffer.resize(2 * chunksize);
        int remainder = stripSize % chunksize;
        for (int j = 0; j < stripSize / chunksize; ++j) {
            int startIndex = j * chunksize;
            file.read(vertexIndexBuffer.data(), 2 * chunksize);
            uint16_t* vertexIndex = reinterpret_cast<uint16_t*>(vertexIndexBuffer.data());
            for (int k = 0; k < chunksize; ++k) {
                strip[startIndex + k] = vertexIndex[k];
            }
        }
        int startIndex = stripSize - remainder;
        for (int j = 0; j < remainder; ++j) {
            file.read(remainderBuffer, sizeof(remainderBuffer));
            strip[startIndex + j] = *reinterpret_cast<uint16_t*>(&remainderBuffer);
        }
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
    char remainderBuffer[2];
    file.read(metadataBuffer, sizeof(metadataBuffer));
    uint16_t numUVs = *reinterpret_cast<uint16_t*>(&metadataBuffer);

    int chunksize = 1;
    if (numUVs >= 256) chunksize = 256;
    else if (numUVs >= 128) chunksize = 128;
    else if (numUVs >= 64) chunksize = 64;
    else if (numUVs >= 32) chunksize = 32;
    else if (numUVs >= 16) chunksize = 16;
    else if (numUVs >= 8) chunksize = 8;
    else if (numUVs >= 4) chunksize = 4;

    std::vector<char> uvCoordBuffer(2 * chunksize);
    mesh->uvs.resize(numUVs);
    float* uvs = mesh->uvs.data();
    int remainder = numUVs % chunksize;
    for (int i = 0; i < numUVs / chunksize; ++i) {
        int uvStartIndex = i * chunksize;
        file.read(uvCoordBuffer.data(), 2 * chunksize);
        uint16_t* uvCoords = reinterpret_cast<uint16_t*>(uvCoordBuffer.data());
        for (int j = 0; j < chunksize; ++j) {
            uvs[uvStartIndex + j] = (float)uvCoords[j] / 10000;
        }
    }
    int startIndex = numUVs - remainder;
    for (int i = 0; i < remainder; ++i) {
        file.read(remainderBuffer, sizeof(remainderBuffer));
        uint16_t* uvCoords = reinterpret_cast<uint16_t*>(&uvCoordBuffer);
        uvs[startIndex + i] = (float)uvCoords[0] / 10000;
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

    int chunksize = 1;
    if (numVertexNormals >= 64) chunksize = 64;
    else if (numVertexNormals >= 32) chunksize = 32;
    else if (numVertexNormals >= 16) chunksize = 16;
    else if (numVertexNormals >= 8) chunksize = 8;
    else if (numVertexNormals >= 4) chunksize = 4;

    std::vector<char> normalBuffer(12 * chunksize);
    char remainderBuffer[12];
    MeshObject::Vertex* vertices = mesh->vertices.data();
    int remainder = numVertexNormals % chunksize;
    for (int i = 0; i < numVertexNormals / chunksize; ++i) {
        file.read(normalBuffer.data(), 12 * chunksize);
        float* normal = reinterpret_cast<float*>(normalBuffer.data());
        for (int j = 0; j < chunksize; ++j) {
            vertices[j].setNormal(normal[(j * 3)], normal[(j * 3) + 1], normal[(j * 3) + 2]);
        }
    }
    for (int i = 0; i < remainder; ++i) {
        file.read(remainderBuffer, 12);
        float* normal = reinterpret_cast<float*>(&remainderBuffer);
        vertices[i].setNormal(normal[0], normal[1], normal[2]);
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
        numBytes += 2 + (stripSize * 2);
        file.write(reinterpret_cast<const char*>(&stripSize), 2);
        for (int j = 0; j < stripSize; ++j) {
            file.write(reinterpret_cast<const char*>(&strip[j]), 2);
        }
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
    uint16_t numUVs = mesh->uvs.size();
    file.write(reinterpret_cast<const char*>(&numUVs), 2);
    for (int i = 0; i < numUVs; ++i) {
        uint16_t uvInt = static_cast<uint16_t>(mesh->uvs[i] * 10000);
        file.write(reinterpret_cast<const char*>(&uvInt), 2);
    }
    mesh->sizeondisk += 2 + (numUVs * 2);
#if _DEBUG
    Timer::end(start, "Wrote (" + std::to_string(numUVs) + ") uv coords (" + std::to_string(2 + (numUVs * 2)) + " bytes): ");
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
    int numVertexNormals = mesh->vertices.size();
    file.write(reinterpret_cast<const char*>(&numVertexNormals), 2);
    int numBytes = numVertexNormals * 12;
    mesh->sizeondisk += numBytes;
    for (int i = 0; i < numVertexNormals; ++i) {
        MeshObject::Normal normal = mesh->vertices[i].normal;
        file.write(reinterpret_cast<const char*>(&normal.x), 4);
        file.write(reinterpret_cast<const char*>(&normal.y), 4);
        file.write(reinterpret_cast<const char*>(&normal.z), 4);
    }
#if _DEBUG
    Timer::end(start, "Wrote (" + std::to_string(numVertexNormals) + ") vertex normals (" + std::to_string(numBytes) + " bytes): ");
#endif
}
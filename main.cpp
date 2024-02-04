#include <iostream>
#include <random>
#include <fstream>
#include <vector>
#include <string>
#include <FBXReader.h>
#include <ModelManager.h>
#include <Timer.hpp>

/// <summary>
/// Command line syntax:
/// modelmaker &lt;input.fbx&gt; &lt;output.whateverextension&gt;
/// </summary>
/// <param name="argc"></param>
/// <param name="argv"></param>
/// <returns></returns>
int main(int argc, char* argv[])
{
#if _DEBUG
    auto start = Timer::begin();
    MeshObject* fbxMesh = FBXReader::readFBXModel("head.fbx");
    ModelManager::writeToDisk(fbxMesh, "head.m");
    MeshObject* readMesh = ModelManager::readModel("head.m");

    Timer::end(start, "Program completed in: ");
#else
    if (argc == 1) {
        std::cout << "syntax: modelmaker <inputfile.fbx> <outputfile.whateverextension>";
        return 0;
    }
    MeshObject* fbxMesh = FBXReader::readFBXModel(argv[1]);
    ModelManager::writeToDisk(fbxMesh, argv[2]);
    MeshObject* readMesh = ModelManager::readModel(argv[2]);
#endif
    return 0;
}
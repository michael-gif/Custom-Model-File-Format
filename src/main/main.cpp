#include <iostream>
#include <model/MeshObject.h>
#include <model/FBXReader.h>
#include <model/ModelManager.h>
#include <util/Timer.hpp>

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
	std::cout << "MSVC Version: " << _MSC_VER << std::endl;
	auto start = Timer::begin();
	MeshObject fbxMesh;
	if (FBXReader::readFBXModel("icobig.fbx", &fbxMesh)) {
		ModelManager::writeToDisk(&fbxMesh, "icobig.m");
		MeshObject readMesh;
		ModelManager::readModel("icobig.m", &readMesh);
		ModelManager::compare(&readMesh, &fbxMesh);
	}

	Timer::end(start, "Program completed in: ");
#else
	if (argc == 1) {
		std::cout << "syntax: modelmaker <inputfile.fbx> <outputfile.whateverextension>";
		return 0;
	}
	MeshObject fbxMesh;
	if (FBXReader::readFBXModel(argv[1], &fbxMesh)) {
		ModelManager::writeToDisk(&fbxMesh, argv[2]);
		MeshObject readMesh;
		ModelManager::readModel(argv[2], &readMesh);
	}
#endif
	return 0;
}
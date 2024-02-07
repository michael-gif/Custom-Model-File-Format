#ifndef SRC_STRIPER_TRIANGLESTRIPGENERATOR_H_
#define SRC_STRIPER_TRIANGLESTRIPGENERATOR_H_

#include <model/MeshObject.h>
#include <fbxsdk.h>

void striper(FbxMesh* inMesh, MeshObject* outMesh);
void striperNew(FbxMesh* inMesh, MeshObject* outMesh);

#endif
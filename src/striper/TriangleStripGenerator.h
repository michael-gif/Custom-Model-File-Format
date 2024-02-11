#ifndef SRC_STRIPER_TRIANGLESTRIPGENERATOR_H_
#define SRC_STRIPER_TRIANGLESTRIPGENERATOR_H_

#include <fbxsdk.h>
#include <model/MeshObject.h>

void striper(FbxMesh* inMesh, MeshObject* outMesh);
void striper2(FbxMesh* inMesh, MeshObject* outMesh);

#endif
#include "Stdafx.h"
#include <iostream>
#include <fbxsdk.h>
#include <model/MeshObject.h>

void striper3(FbxMesh* inMesh, MeshObject* outMesh)
{
	int* vertices = inMesh->GetPolygonVertices();
	std::vector<uint32_t> faces(inMesh->GetPolygonCount() * 3);
	uint32_t* ptr = faces.data();
	for (int i = 0; i < faces.size(); i++) {
		ptr[i] = vertices[i];
	}
	StriperOptions sc;
	sc.DFaces = ptr;
	sc.faceCount = inMesh->GetPolygonCount();
	sc.askforUINT16 = true;
	sc.connectAllStrips = false;
	sc.oneSided = false;
	sc.SGIAlgorithm = true;

	Striper Strip;
	Strip.init(sc);

	StriperResult sr;
	Strip.compute(sr);

	std::cout << "Number of strips: " << sr.stripCount << std::endl;
	//int numTris = 0;
	//for(uint32_t i = 0; i < sr.stripCount; i++)
	//{
	//	uint32_t stripLength = sr.stripLengths[i];
	//	numTris += stripLength - 2;
	//	std::cout << stripLength << "\n";
	//}
	//std::cout << "Num Tris: " << numTris << std::endl;
	uint16_t* indices = (uint16_t*)sr.stripIndices;
	for (int i = 0; i < sr.stripLengths[0]; i++) {
		std::cout << *indices << " ";
		indices++;
	}
	std::cout << std::endl;
}

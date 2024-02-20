#ifndef SRC_MODEL_MESHOBJECT_H_
#define SRC_MODEL_MESHOBJECT_H_

#include <iostream>
#include <vector>

class MeshObject {
public:
    MeshObject() {
        triangleStrips = new std::vector<std::vector<uint16_t>>();
    }
    ~MeshObject() {
        delete triangleStrips;
    }
    int sizeondisk = 0;

    struct Normal {
        float x = 0;
        float y = 0;
        float z = 0;

        Normal() {}
        Normal(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}
    };

    struct Vertex {
        float x = 0;
        float y = 0;
        float z = 0;
        Normal normal;

        Vertex() {}
        Vertex(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}

        void prntString() {
            std::cout << "(" << x << "," << y << "," << z << ")" << std::endl;
        }

        void setPos(float _x, float _y, float _z) {
            x = _x;
            y = _y;
            z = _z;
        }

        void setNormal(float x, float y, float z) {
            normal.x = x;
            normal.y = y;
            normal.z = z;
        }
    };

    struct Triangle {
        int vertex1 = 0;
        int vertex2 = 0;
        int vertex3 = 0;

        Triangle() {}
        Triangle(int v1, int v2, int v3) : vertex1(v1), vertex2(v2), vertex3(v3) {}
    };

    struct UVCoord {
        float x = 0;
        float y = 0;

        UVCoord() {}
        UVCoord(float _x, float _y) : x(_x), y(_y) {}
    };

    std::vector<Vertex> vertices;
    std::vector<uint16_t> vertexIndices;
    std::vector<uint32_t> edges;
    std::vector<std::vector<uint16_t>>* triangleStrips;
    std::vector<float> uvs;
    std::vector<std::vector<float>> normalStrips;
};

#endif
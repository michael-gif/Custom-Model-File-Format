#pragma once
#ifndef MESH_OBJECT_HPP
#define MESH_OBJECT_HPP

#include <iostream>
#include <vector>

class MeshObject {
private:
    struct Normal {
        float x = 0;
        float y = 0;
        float z = 0;

        Normal() {}
        Normal(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}
    };
public:
    int sizeondisk = 0;

    struct Vertex {
        float x = 0;
        float y = 0;
        float z = 0;

        Vertex() {}
        Vertex(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}

        void prntString() {
            std::cout << "(" << x << "," << y << "," << z << ")" << std::endl;
        }
    };

    struct Triangle {
        int vertex1 = 0;
        int vertex2 = 0;
        int vertex3 = 0;
        Normal normal;

        Triangle() {}
        Triangle(int v1, int v2, int v3) : vertex1(v1), vertex2(v2), vertex3(v3) {}

        void setNormal(float x, float y, float z) {
            normal.x = x;
            normal.y = y;
            normal.z = z;
        }
    };

    struct UVCoord {
        float x = 0;
        float y = 0;

        UVCoord() {}
        UVCoord(float _x, float _y) : x(_x), y(_y) {}
    };

    std::vector<Vertex> vertices;
    std::vector<std::vector<int>> triangleStrips;
    std::vector<std::vector<float>> uvStrips;
    std::vector<std::vector<float>> normalStrips;
};

#endif
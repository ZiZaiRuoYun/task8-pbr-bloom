#pragma once
class Mesh {
public:
    void InitBuffers();
    unsigned int GetVAO() const { return vao; }
    unsigned int GetVertexCount() const { return vertexCount; }
private:
    unsigned int vao = 0;
    unsigned int vbo = 0;
    unsigned int vertexCount = 0;
};
#include "Mesh.h"
#include <glad/glad.h>
#include <glm/glm.hpp>

void Mesh::InitBuffers() {
    // positions (x,y,z) + normals (nx,ny,nz)  => 6 floats per vertex
    float vertices[] = {
        // ----- back face (z = -0.5), normal (0,0,-1)
        -0.5f,-0.5f,-0.5f,   0.0f, 0.0f,-1.0f,
         0.5f,-0.5f,-0.5f,   0.0f, 0.0f,-1.0f,
         0.5f, 0.5f,-0.5f,   0.0f, 0.0f,-1.0f,
         0.5f, 0.5f,-0.5f,   0.0f, 0.0f,-1.0f,
        -0.5f, 0.5f,-0.5f,   0.0f, 0.0f,-1.0f,
        -0.5f,-0.5f,-0.5f,   0.0f, 0.0f,-1.0f,

        // ----- front face (z = +0.5), normal (0,0,1)
        -0.5f,-0.5f, 0.5f,   0.0f, 0.0f, 1.0f,
         0.5f,-0.5f, 0.5f,   0.0f, 0.0f, 1.0f,
         0.5f, 0.5f, 0.5f,   0.0f, 0.0f, 1.0f,
         0.5f, 0.5f, 0.5f,   0.0f, 0.0f, 1.0f,
        -0.5f, 0.5f, 0.5f,   0.0f, 0.0f, 1.0f,
        -0.5f,-0.5f, 0.5f,   0.0f, 0.0f, 1.0f,

        // ----- left face (x = -0.5), normal (-1,0,0)
        -0.5f,-0.5f,-0.5f,  -1.0f, 0.0f, 0.0f,
        -0.5f,-0.5f, 0.5f,  -1.0f, 0.0f, 0.0f,
        -0.5f, 0.5f, 0.5f,  -1.0f, 0.0f, 0.0f,
        -0.5f, 0.5f, 0.5f,  -1.0f, 0.0f, 0.0f,
        -0.5f, 0.5f,-0.5f,  -1.0f, 0.0f, 0.0f,
        -0.5f,-0.5f,-0.5f,  -1.0f, 0.0f, 0.0f,

        // ----- right face (x = +0.5), normal (1,0,0)
         0.5f,-0.5f,-0.5f,   1.0f, 0.0f, 0.0f,
         0.5f,-0.5f, 0.5f,   1.0f, 0.0f, 0.0f,
         0.5f, 0.5f, 0.5f,   1.0f, 0.0f, 0.0f,
         0.5f, 0.5f, 0.5f,   1.0f, 0.0f, 0.0f,
         0.5f, 0.5f,-0.5f,   1.0f, 0.0f, 0.0f,
         0.5f,-0.5f,-0.5f,   1.0f, 0.0f, 0.0f,

         // ----- bottom face (y = -0.5), normal (0,-1,0)
         -0.5f,-0.5f,-0.5f,   0.0f,-1.0f, 0.0f,
          0.5f,-0.5f,-0.5f,   0.0f,-1.0f, 0.0f,
          0.5f,-0.5f, 0.5f,   0.0f,-1.0f, 0.0f,
          0.5f,-0.5f, 0.5f,   0.0f,-1.0f, 0.0f,
         -0.5f,-0.5f, 0.5f,   0.0f,-1.0f, 0.0f,
         -0.5f,-0.5f,-0.5f,   0.0f,-1.0f, 0.0f,

         // ----- top face (y = +0.5), normal (0,1,0)
         -0.5f, 0.5f,-0.5f,   0.0f, 1.0f, 0.0f,
          0.5f, 0.5f,-0.5f,   0.0f, 1.0f, 0.0f,
          0.5f, 0.5f, 0.5f,   0.0f, 1.0f, 0.0f,
          0.5f, 0.5f, 0.5f,   0.0f, 1.0f, 0.0f,
         -0.5f, 0.5f, 0.5f,   0.0f, 1.0f, 0.0f,
         -0.5f, 0.5f,-0.5f,   0.0f, 1.0f, 0.0f
    };

    vertexCount = 36;

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // pos -> location 0
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);

    // normal -> location 1
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));

    glBindVertexArray(0);

}

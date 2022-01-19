#pragma once

#include "opengl_helpers.h"

class asteroid_mesh
{
public:
    asteroid_mesh(GL::cache& GLCache);

    void Draw(int instance);

    // Mesh
    GLuint VBO = 0;
    GLuint VAO = 0;
    int MeshVertexCount = 0;
    vertex_descriptor MeshDesc;
    // Textures
    GLuint DiffuseTexture = 0;

private:

};
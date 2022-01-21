
#include "asteroid_mesh.h"

asteroid_mesh::asteroid_mesh(GL::cache& GLCache)
{
    // Create mesh
    {
        // Use vbo from GLCache
        VBO = GLCache.LoadObj("media/rock.obj", 1.f, &MeshVertexCount, &MeshDesc);

        glGenVertexArrays(1, &VAO);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, MeshDesc.Stride, (void*)(size_t)MeshDesc.PositionOffset);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, MeshDesc.Stride, (void*)(size_t)MeshDesc.UVOffset);
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, MeshDesc.Stride, (void*)(size_t)MeshDesc.NormalOffset);
    }

    // Gen texture
    {
        DiffuseTexture = GLCache.LoadTexture("media/rock.png", IMG_FLIP | IMG_GEN_MIPMAPS);
    }
}

asteroid_mesh::~asteroid_mesh()
{
    glDeleteBuffers(1, &VAO);
    glDeleteBuffers(1, &VBO);
}


void asteroid_mesh::Draw(int instance)
{
    glBindTexture(GL_TEXTURE_2D, DiffuseTexture);
    // Draw mesh
    glBindVertexArray(VAO);
    glDrawArraysInstanced(GL_TRIANGLES, 0, MeshVertexCount, instance);
}
#pragma once

#include <array>
#include <vector>

#include "demo.h"

#include "opengl_headers.h"

#include "camera.h"

#include "tavern_scene.h"

#define INSTANCE 10000

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

class demo_instancing : public demo
{
public:
    demo_instancing(GL::cache& GLCache, GL::debug& GLDebug);
    virtual ~demo_instancing();
    virtual void Update(const platform_io& IO);

    void RenderScene(const mat4& ProjectionMatrix, const mat4& ViewMatrix, const mat4& ModelMatrix);
    void DisplayDebugUI();
    void InspectLights();
    void RenderQuad();
    void RenderAsteroids();


    GLuint LightsUniformBuffer;
    int LightCount = 8;
    int InstanceCount = 500;

private:
    
    void GenMatrices();
    
    GL::debug& GLDebug;
    std::vector<GL::light> Lights;

    // 3d camera
    camera Camera = {};

    // GL objects needed by this demo
    GLuint Program = 0;
    GLuint quadVAO = 0;
    GLuint Texture = 0;

    asteroid_mesh asteroid;

    bool Wireframe = false;
};

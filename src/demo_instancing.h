#pragma once

#include <array>

#include "demo.h"

#include "opengl_headers.h"

#include "camera.h"

#include "tavern_scene.h"

#define INSTANCE 1000
#define ASTEROID_FIELD

class asteroid_mesh
{
public:
    asteroid_mesh(GL::cache& GLCache);

    void Draw(float VAO);

    // Mesh
    GLuint MeshBuffer = 0;
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

private:
    GL::debug& GLDebug;
    std::vector<GL::light> Lights;

#ifdef ASTEROID_FIELD
    mat4 modelMatrices[INSTANCE];
#endif

    // 3d camera
    camera Camera = {};

    // GL objects needed by this demo
    GLuint Program = 0;
    GLuint quadVAO = 0;
    GLuint Texture = 0;

    asteroid_mesh asteroid;
    GLuint asteroidVAO = 0;

    bool Wireframe = false;
};

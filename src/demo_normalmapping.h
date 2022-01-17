#pragma once

#include <array>

#include "demo.h"

#include "opengl_headers.h"

#include "camera.h"

#include "tavern_scene.h"

class bag_object
{
public:
    bag_object(GL::cache& GLCache);

    // Mesh
    GLuint MeshBuffer = 0;
    GLuint MeshArrayObject = 0;
    int MeshVertexCount = 0;
    vertex_descriptor MeshDesc;

    // Textures
    GLuint DiffuseTexture = 0;
    GLuint NormalTexture = 0;
};

class demo_normalmapping : public demo
{
public:
    demo_normalmapping(GL::cache& GLCache, GL::debug& GLDebug);
    virtual ~demo_normalmapping();
    virtual void Update(const platform_io& IO);

    void RenderScene(const mat4& ProjectionMatrix, const mat4& ViewMatrix, const mat4& ModelMatrix);
    void DisplayDebugUI();
    void InspectLights();
    void RenderQuad();
    void RenderBag();


    GLuint LightsUniformBuffer;
    int LightCount = 8;

private:
    GL::debug& GLDebug;
    std::vector<GL::light> Lights;


    // 3d camera
    camera Camera = {};

    // GL objects needed by this demo
    GLuint Program = 0;

    bag_object BagObject;

    GLuint quadVAO = 0;
    GLuint Texture = 0;
    GLuint NormalTexture = 0;

    v3 Position = { 0, 0, -2.f };
    v3 Rotation = { 0, 0, 0 };
    float Scale = 0.01f;

    bool Wireframe = false;
    bool NormalMapping = false;
};

#pragma once

#include <array>
#include <vector>

#include "demo.h"

#include "opengl_headers.h"

#include "camera.h"

#include "asteroid_mesh.h"


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

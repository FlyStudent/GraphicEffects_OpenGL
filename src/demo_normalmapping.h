#pragma once

#include <array>

#include "demo.h"

#include "opengl_headers.h"

#include "camera.h"

#include "tavern_scene.h"

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


    GLuint LightsUniformBuffer;
    int LightCount = 8;

private:
    GL::debug& GLDebug;
    std::vector<GL::light> Lights;


    // 3d camera
    camera Camera = {};

    // GL objects needed by this demo
    GLuint Program = 0;
    GLuint quadVAO = 0;
    GLuint Texture = 0;

    bool Wireframe = false;
};

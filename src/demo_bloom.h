#pragma once

#include <array>

#include "demo_base.h"

#include "opengl_headers.h"

#include "camera.h"

#include "tavern_scene.h"

class demo_bloom : public demo_base
{
public:
    demo_bloom(GL::cache& GLCache, GL::debug& GLDebug, const platform_io& IO);
    virtual ~demo_bloom();
    virtual void Update(const platform_io& IO);

    void RenderTavern(const mat4& ProjectionMatrix, const mat4& ViewMatrix, const mat4& ModelMatrix);
    void DisplayDebugUI();
    void RenderQuad();

private:
    GL::debug& GLDebug;

    // 3d camera
    camera Camera = {};

    // GL objects needed by this demo
    GLuint Program = 0;
    GLuint VAO = 0;
    GLuint quadVAO = 0;

    GLuint blurProgram = 0;

    unsigned int postProcessingFBO;
    unsigned int colorBuffer;

    unsigned int pingpongFBO[2];
    unsigned int pingpongBuffer[2];

    tavern_scene TavernScene;

    bool Wireframe = false;
};

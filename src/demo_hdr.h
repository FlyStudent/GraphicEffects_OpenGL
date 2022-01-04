#pragma once

#include <array>

#include "demo.h"

#include "opengl_headers.h"

#include "camera.h"

#include "tavern_scene.h"

class demo_hdr : public demo
{
public:
    demo_hdr(GL::cache& GLCache, GL::debug& GLDebug, const platform_io& IO);
    virtual ~demo_hdr();
    virtual void Update(const platform_io& IO);

    void RenderTavern(const mat4& ProjectionMatrix, const mat4& ViewMatrix, const mat4& ModelMatrix);
    void DisplayDebugUI();

private:
    GL::debug& GLDebug;

    // 3d camera
    camera Camera = {};

    // GL objects needed by this demo
    GLuint Program = 0;
    GLuint VAO = 0;

    // HDR objects
    GLuint hdrBuffer = 0;
    GLuint colorBuffer = 0;

    tavern_scene TavernScene;

    bool Wireframe = false;
};

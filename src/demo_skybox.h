#pragma once

#include <array>

#include "demo_base.h"

#include "opengl_headers.h"

#include "camera.h"

#include "tavern_scene.h"

class demo_skybox : public demo_base
{
public:
    demo_skybox(GL::cache& GLCache, GL::debug& GLDebug);
    virtual ~demo_skybox();
    virtual void Update(const platform_io& IO);

    void DisplayDebugUI();

private:
    GL::debug& GLDebug;

    // 3d camera
    camera Camera = {};

    // GL objects needed by this demo
    GLuint Program = 0;
    GLuint VAO = 0;

    GLuint SkyProgram = 0;
    GLuint SkyVAO = 0;
    GLuint SkyBuffer = 0;
    GLuint SkyTexture = 0;

    bool Wireframe = false;
};
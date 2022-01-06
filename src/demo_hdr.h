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

    void RenderQuad();
    void RenderTavern(const mat4& ProjectionMatrix, const mat4& ViewMatrix, const mat4& ModelMatrix);
    void DisplayDebugUI();

private:
    GL::debug& GLDebug;

    // 3d camera
    camera Camera = {};

    // GL objects needed by this demo
    GLuint Program = 0;
    GLuint VAO = 0;
    GLuint quadVAO = 0;

    GLuint blurProgram = 0;

    // HDR objects
    bool processHdr = true;
    bool processGamma = false;
    float exposure = 1.f;

    unsigned int pingpongFBO[2];
    unsigned int pingpongCBO[2];

    GLuint hdrProgram = 0;
    GLuint FBO = 0;
    GLuint RBO = 0;
    GLuint hdrCBO = 0;
    GLuint bloomCBO = 0;

    tavern_scene TavernScene;

    bool Wireframe = false;
};

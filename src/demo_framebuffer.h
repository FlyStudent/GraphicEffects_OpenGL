#pragma once

#include <array>

#include "demo.h"

#include "post_process_type.h"
#include "opengl_headers.h"

#include "camera.h"

#include "tavern_scene.h"



class demo_framebuffer : public demo
{
public:
    demo_framebuffer(GL::cache& GLCache, GL::debug& GLDebug, const platform_io& IO);
    virtual ~demo_framebuffer();
    virtual void Update(const platform_io& IO);

    void RenderTavern(const mat4& ProjectionMatrix, const mat4& ViewMatrix, const mat4& ModelMatrix);
    void DisplayDebugUI();

private:
    GL::debug& GLDebug;

    // 3d camera
    camera Camera = {};

    // GL objects needed by this demo
    GLuint Program = 0;
    GLuint FramebufferProgram = 0;
    unsigned int quadVAO = 0;
    unsigned int tavernVAO = 0;
    unsigned int quadVBO = 0;

    unsigned int FBO;
    unsigned int framebufferTexture;
    unsigned int RBO;

    tavern_scene TavernScene;

    bool ratioXYkernel = true;
    float x_ratio_kernel = 800.f;
    float y_ratio_kernel = 800.f;
    bool processGreyScale = false;
    bool processInverse = false;
    bool processKernel = false;
    mat3    kernelMat = {
            0,0,0,
            0,1,0,
            0,0,0
    };

    bool Wireframe = false;
};

#pragma once

#include <array>

#include "demo.h"

#include "post_process_type.h"
#include "opengl_headers.h"

#include "camera.h"

#include "tavern_scene.h"

class demo_full : public demo
{
public:
    demo_full(GL::cache& GLCache, GL::debug& GLDebug, const platform_io& IO);
    virtual ~demo_full();
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

    GLuint FBO = 0;
    GLuint RBO = 0;
    GLuint CBO = 0;

    // HDR objects
    GLuint PostProcessProgram = 0;

    bool processHdr = true;
    bool processGamma = true;

    float gamma = 2.2f;
    float exposure = 1.f;

    GLuint bloomCBO = 0;
    GLuint blurProgram = 0;
    bool processBloom = true;

    int pingpongAmount = 8;
    unsigned int pingpongFBO[2];
    unsigned int pingpongCBO[2];
    float brightnessClamp = 0.5f;

    // Post process Objects
    bool processGreyScale = false;
    bool processInverse = false;
    bool processKernel = false;
    mat3    kernelMat = {
            0,0,0,
            0,1,0,
            0,0,0
    };


    tavern_scene TavernScene;

    PostProcessType postProcessType = PostProcessType::GREYSCALE;

    bool Wireframe = false;
};

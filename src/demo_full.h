#pragma once

#include <array>

#include "demo.h"

#include "post_process_type.h"
#include "opengl_headers.h"

#include "camera.h"

#include "asteroid_mesh.h"
#include "tavern_scene.h"

class demo_full : public demo
{
public:
    demo_full(GL::cache& GLCache, GL::debug& GLDebug, const platform_io& IO);
    virtual ~demo_full();
    virtual void Update(const platform_io& IO);

    void RenderQuad();
    void RenderTavern(const mat4& ProjectionMatrix, const mat4& ViewMatrix, const mat4& ModelMatrix);
    void RenderAsteroids(const mat4& ProjectionMatrix, const mat4& ViewMatrix, const mat4& ModelMatrix);
    void RenderReflectiveSphere(const mat4& ProjectionMatrix, const mat4& ViewMatrix, const mat4& ModelMatrix);
    void RenderScene(const camera& cam = {}, bool reflection = true);
    void RenderSkybox(const camera& cam, const mat4& projection);
    void RenderEnvironmentMap();
    void DisplayDebugUI();


private:
    void GenCubemap(GLuint& index, const float width, const float height, const GLint format, const GLint size);
    void GenInstanceMatrices();
    void GenInstanceVBO(const std::vector<mat4>& matrices);

    GL::debug& GLDebug;

    // 3d camera
    camera Camera = {};
    // rendering camera
    camera RenderingCamera = {};

    // GL objects needed by this demo
    GLuint Program = 0;
    GLuint VAO = 0;
    GLuint quadVAO = 0;
    GLuint SphereVAO = 0;

    const int renderIndex = 0, hdrIndex = 1;
    GLuint FBOs[2];
    GLuint RBOs[2];
    GLuint CBOs[2];


    float AspectRatio = 0.f;

    // Skybox
    GLuint SkyFBO = 0;
    GLuint SkyProgram = 0;
    GLuint SkyVAO = 0;
    GLuint SkyTexture = 0;

    GLuint ReflectiveProgram = 0;
    GLuint EnvironmentTexture = 0;
    bool Skybox = true;
    bool Dynamic = true;


    // HDR objects

    GLuint HdrProgram = 0;
    bool processHdr = true;
    bool processGamma = true;

    float gamma = 2.2f;
    float exposure = 1.f;

    GLuint bloomCBO = 0;
    GLuint BlurProgram = 0;
    bool processBloom = true;

    int pingpongAmount = 8;
    GLuint pingpongFBO[2];
    GLuint pingpongCBO[2];
    float brightnessClamp = 0.5f;

    // Post process Objects
    GLuint PostProcessProgram = 0;

    bool processGreyScale = false;
    bool processInverse = false;
    bool processKernel = false;
    bool ratioXYkernel = true;

    float x_ratio_kernel = 800.f;
    float y_ratio_kernel = 800.f;

    mat3    kernelMat = {
            0,0,0,
            0,1,0,
            0,0,0
    };

    // Instancing Objects
    GLuint InstancingProgram = 0;
    asteroid_mesh asteroid;

    bool processInstancing = true;
    int instanceCount = 500;
    float instanceCircleRadius = 15.f;
    float instanceOffset = 5.f;


    tavern_scene TavernScene;

    bool Wireframe = false;
};

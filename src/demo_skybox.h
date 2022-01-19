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

    void RenderEnvironmentMap(const v3& center);
    void RenderScene(const mat4& ProjectionMatrix, const mat4& ModelMatrix, const v3& position, const camera& cam = {});
    void RenderSceneWithReflection(const mat4& ProjectionMatrix, const mat4& ModelMatrix, const camera& cam);
    void GenerateCubemap(GLuint& index, const float width, const float height, const GLint format, const GLint size);

    void RenderTavernEx(const mat4& ProjectionMatrix, const mat4& ViewMatrix, const mat4& ModelMatrix,int i);

    void MousePicking(const platform_io& IO);

    void RenderDepthMap();
private:
    GL::debug& GLDebug;

    GLuint VAO = 0;

    GLuint SphereVAO = 0;
    GLuint SphereBuffer = 0;

    GLuint SkyProgram = 0;
    GLuint SkyVAO = 0;
    GLuint SkyBuffer = 0;

    void RenderSkybox(const camera& cam, const mat4& projection);

    bool Wireframe = false;

    // GL objects needed by this demo
    GLuint ReflectiveProgram = 0;

    GLuint SkyTexture = 0;
    GLuint EnvironmentTexture = 0;
    GLuint DepthTexture = 0;

    GLuint MousePickingProgram = 0;

    GLuint CubeVAO = 0;

    v3 Position = { 0.f,0.f,0.f };
    bool Dynamic = true;
};
#pragma once

#include <array>

#include "demo.h"

#include "opengl_headers.h"

#include "camera.h"

#include "tavern_scene.h"

class demo_base : public demo
{
public:
    demo_base(GL::cache& GLCache, GL::debug& GLDebug);
    virtual ~demo_base();
    virtual void Update(const platform_io& IO);

    void RenderTavern(const mat4& ProjectionMatrix, const mat4& ViewMatrix, const mat4& ModelMatrix);
    void DisplayDebugUI();

private:
    GL::debug& GLDebug;

    GLuint VAO = 0;

    bool Wireframe = false;

protected:

    // GL objects needed by this demo
    GLuint Program = 0;

    // 3d camera
    camera Camera = {};

    // rendering camera
    camera RenderingCamera = {};
    
    tavern_scene TavernScene;

};

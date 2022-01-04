#pragma once

#include "demo.h"

#include "opengl_headers.h"

#include "camera.h"

class demo_hdr : public demo
{
public:
    demo_hdr();
    virtual ~demo_hdr();
    virtual void Update(const platform_io& IO);

private:
    // 3d camera
    camera Camera = {};
    
    // GL objects needed by this demo
    GLuint Program = 0;
    GLuint Texture = 0;

    GLuint VAO = 0;
    GLuint VertexBuffer = 0;
    int VertexCount = 0;

};

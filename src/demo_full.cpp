
#include <vector>
#include <iostream>

#include <imgui.h>

#include "stb_image.h"

#include "opengl_helpers.h"
#include "opengl_helpers_wireframe.h"

#include "color.h"
#include "maths.h"
#include "mesh.h"

#include "demo_full.h"

const int LIGHT_BLOCK_BINDING_POINT = 0;

// Vertex format
// ==================================================
struct vertex
{
    v3 Position;
    v2 UV;
};

#pragma region BASIC VS
static const char* gVertexShaderStr = R"GLSL(
// Attributes
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aUV;
layout(location = 2) in vec3 aNormal;

// Uniforms
uniform mat4 uProjection;
uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uModelNormalMatrix;

// Varyings
out vec2 vUV;
out vec3 vPos;    // Vertex position in view-space
out vec3 vNormal; // Vertex normal in view-space

void main()
{
    vUV = aUV;
    vec4 pos4 = (uModel * vec4(aPosition, 1.0));
    vPos = pos4.xyz / pos4.w;
    vNormal = (uModelNormalMatrix * vec4(aNormal, 0.0)).xyz;
    gl_Position = uProjection * uView * pos4;
})GLSL";
#pragma endregion

#pragma region BASIC FS
static const char* gFragmentShaderStr = R"GLSL(
// Varyings
in vec2 vUV;
in vec3 vPos;
in vec3 vNormal;

// Uniforms
uniform mat4 uProjection;
uniform vec3 uViewPosition;
uniform float uBrightness;

uniform sampler2D uDiffuseTexture;
uniform sampler2D uEmissiveTexture;

// Uniform blocks
layout(std140) uniform uLightBlock
{
	light uLight[LIGHT_COUNT];
};

// Shader outputs
//out vec4 oColor;
layout (location = 0) out vec4 oColor;
layout (location = 1) out vec4 BloomoColor;

light_shade_result get_lights_shading()
{
    light_shade_result lightResult = light_shade_result(vec3(0.0), vec3(0.0), vec3(0.0));
	for (int i = 0; i < LIGHT_COUNT; ++i)
    {
        light_shade_result light = light_shade(uLight[i], gDefaultMaterial.shininess, uViewPosition, vPos, normalize(vNormal));
        lightResult.ambient  += light.ambient;
        lightResult.diffuse  += light.diffuse;
        lightResult.specular += light.specular;
    }
    return lightResult;
}

void main()
{
    // Compute phong shading
    light_shade_result lightResult = get_lights_shading();
    
    vec3 diffuseColor  = gDefaultMaterial.diffuse * lightResult.diffuse * texture(uDiffuseTexture, vUV).rgb;
    vec3 ambientColor  = gDefaultMaterial.ambient * lightResult.ambient;
    vec3 specularColor = gDefaultMaterial.specular * lightResult.specular;
    vec3 emissiveColor = gDefaultMaterial.emission + texture(uEmissiveTexture, vUV).rgb;
    
    // Apply light color
    oColor = vec4((ambientColor + diffuseColor + specularColor + emissiveColor), 1.0);
    
    float brightness = dot(oColor.rgb, vec3(0.2126, 0.7152, 0.0722));
    if(brightness > uBrightness)
        BloomoColor = vec4(oColor.rgb, 1.0);
    else
       BloomoColor = vec4(0.0f, 0.0f, 0.0f, 1.0f);
})GLSL";
#pragma endregion

#pragma region CUBE VS
static const char* gVertexShaderCubeStr = R"GLSL(
layout (location = 0) in vec3 aPos;

out vec3 TexCoords;

uniform mat4 projection;
uniform mat4 view;

void main()
{
    TexCoords = aPos;
    gl_Position = projection * view * vec4(aPos, 1.0);
}  )GLSL";
#pragma endregion

#pragma region CUBE FS
static const char* gFragmentShaderCubeStr = R"GLSL(
out vec4 FragColor;

in vec3 TexCoords;

uniform samplerCube skybox;

void main()
{    
    FragColor = texture(skybox, TexCoords);
})GLSL";
#pragma endregion

#pragma region REFLECTION FS
static const char* gReflectionFragmentShaderStr = R"GLSL(
// Varyings
in vec2 vUV;
in vec3 vPos;
in vec3 vNormal;

// Uniforms
uniform mat4 uProjection;
uniform vec3 uViewPosition;

uniform sampler2D uDiffuseTexture;
uniform sampler2D uEmissiveTexture;

uniform samplerCube skybox;

// Uniform blocks
layout(std140) uniform uLightBlock
{
	light uLight[LIGHT_COUNT];
};

// Shader outputs
out vec4 oColor;

light_shade_result get_lights_shading()
{
    light_shade_result lightResult = light_shade_result(vec3(0.0), vec3(0.0), vec3(0.0));
	for (int i = 0; i < LIGHT_COUNT; ++i)
    {
        light_shade_result light = light_shade(uLight[i], gDefaultMaterial.shininess, uViewPosition, vPos, normalize(vNormal));
        lightResult.ambient  += light.ambient;
        lightResult.diffuse  += light.diffuse;
        lightResult.specular += light.specular;
    }
    return lightResult;
}

void main()
{
    // Compute phong shading
    light_shade_result lightResult = get_lights_shading();
    
    vec3 diffuseColor  = gDefaultMaterial.diffuse * lightResult.diffuse * texture(uDiffuseTexture, vUV).rgb;
    vec3 ambientColor  = gDefaultMaterial.ambient * lightResult.ambient;
    vec3 specularColor = gDefaultMaterial.specular * lightResult.specular;
    vec3 emissiveColor = gDefaultMaterial.emission + texture(uEmissiveTexture, vUV).rgb;
    
    // Apply light color
    oColor = vec4((ambientColor + diffuseColor + specularColor + emissiveColor), 1.0);

    vec3 I = normalize(vPos - uViewPosition);
    vec3 R = reflect(I, normalize(vNormal));
    float ratio = 1.00/1.52;
    //R = refract(I, normalize(vNormal), ratio);

    oColor = vec4(texture(skybox, R).rgb, 1.0);
})GLSL";
#pragma endregion

#pragma region INSTANCING VS
static const char* gInstancingVertexShaderStr = R"GLSL(
// Attributes
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aUV;
layout(location = 2) in vec3 aNormal;
layout(location = 3) in mat4 aInstanceMatrix;

// Uniforms
uniform mat4 uProjection;
uniform mat4 uView;
uniform mat4 uModelNormalMatrix;

// Varyings
out vec2 vUV;
out vec3 vPos;    // Vertex position in view-space
out vec3 vNormal; // Vertex normal in view-space

void main()
{
    vUV = aUV;
    vec4 pos4 = (aInstanceMatrix * vec4(aPosition, 1.0));
    vPos = pos4.xyz / pos4.w;
    vNormal = (uModelNormalMatrix * vec4(aNormal, 0.0)).xyz;
    gl_Position = uProjection * uView * pos4;
})GLSL";
#pragma endregion

#pragma region INSTANCING FS
static const char* gInstancingFragmentShaderStr = R"GLSL(
#line 145
// Varyings
in vec2 vUV;
in vec3 vPos;
in vec3 vNormal;

// Uniforms
uniform mat4 uProjection;
uniform vec3 uViewPosition;

uniform sampler2D uDiffuseTexture;

// Uniform blocks
layout(std140) uniform uLightBlock
{
	light uLight[LIGHT_COUNT];
};

// Shader outputs
out vec4 oColor;

light_shade_result get_lights_shading()
{
    light_shade_result lightResult = light_shade_result(vec3(0.0), vec3(0.0), vec3(0.0));
	for (int i = 0; i < LIGHT_COUNT; ++i)
    {
        light_shade_result light = light_shade(uLight[i], gDefaultMaterial.shininess, uViewPosition, vPos, normalize(vNormal));
        lightResult.ambient  += light.ambient;
        lightResult.diffuse  += light.diffuse;
        lightResult.specular += light.specular;
    }
    return lightResult;
}

void main()
{
    // Compute phong shading
    light_shade_result lightResult = get_lights_shading();
    
    vec3 diffuseColor  = gDefaultMaterial.diffuse * lightResult.diffuse * texture(uDiffuseTexture, vUV).rgb;
    vec3 ambientColor  = gDefaultMaterial.ambient * lightResult.ambient;
    vec3 specularColor = gDefaultMaterial.specular * lightResult.specular;
    vec3 emissiveColor = gDefaultMaterial.emission;
    
    // Apply light color
    oColor = vec4((ambientColor + diffuseColor + specularColor + emissiveColor), 1.0);
})GLSL";
#pragma endregion

#pragma region HDR VS
static const char* gHdrVertexShaderStr = R"GLSL(
// Attributes
layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec2 aUV;

// shader output
out vec2 TexCoords;

void main()
{
    gl_Position = vec4(aPosition, 1.0);
    TexCoords = aUV;
})GLSL";
#pragma endregion

#pragma region HDR FS
static const char* gHdrFragmentShaderStr = R"GLSL(
// shader inputs
in vec2 TexCoords;

// uniforms
uniform sampler2D uScreenTexture;
uniform sampler2D uBloomTexture;

uniform bool uProcessHdr;
uniform bool uProcessGamma;
uniform bool uProcessBloom;
uniform float uGamma;
uniform float uExposure;

uniform bool uProcessGreyScale;
uniform bool uProcessInverse;
uniform bool uProcessKernel;

uniform mat3 uKernel;
uniform float x_ratio;
uniform float y_ratio;

// shader ouputs
out vec4 FragColor;

void PostProcess()
{
    float offset_x = 1.0f / x_ratio;
    float offset_y = 1.0f / y_ratio;

    vec2 offsets[9] = vec2[](
        vec2(-offset_x,  offset_y), // top-left
        vec2( 0.0f,    offset_y), // top-center
        vec2( offset_x,  offset_y), // top-right
        vec2(-offset_x,  0.0f),   // center-left
        vec2( 0.0f,    0.0f),   // center-center
        vec2( offset_x,  0.0f),   // center-right
        vec2(-offset_x, -offset_y), // bottom-left
        vec2( 0.0f,   -offset_y), // bottom-center
        vec2( offset_x, -offset_y)  // bottom-right    
    );

    if (uProcessKernel)
    {
        vec3 color = vec3(0.0f);
        for(int i = 0; i < 3; i++)
            for(int j = 0; j < 3; j++)
                color += vec3(texture(uScreenTexture, TexCoords.st + offsets[i*3 + j])) * uKernel[i][j];

        FragColor = vec4(color, 1.0f);
    }

    if (uProcessInverse)
        FragColor = vec4(1.0f) - FragColor;                         

    if (uProcessGreyScale)
    {
        float average = (FragColor.r + FragColor.g + FragColor.b) / 3.f; 
        FragColor = vec4(average, average, average, 1.0f);
    }
}

void main()
{
	vec3 hdrColor = texture(uScreenTexture, TexCoords).rgb;
    vec3 bloomColor = texture(uBloomTexture, TexCoords).rgb;
    
    if (uProcessBloom)
        hdrColor = hdrColor + bloomColor;

	if (uProcessHdr)
	{
		vec3 toneMapped = vec3(1.0) - exp(-hdrColor * uExposure);
        hdrColor = toneMapped;
	}
    // Gamma correction
    if (uProcessGamma)
	    hdrColor = pow(hdrColor, vec3(1.0/uGamma));
    
    FragColor = vec4(hdrColor, 1.0);

    PostProcess();

})GLSL";
#pragma endregion

#pragma region BLUR FS
static const char* BlurFragmentShaderStr = R"GLSL(
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform bool horizontal;
uniform float weight[5] = float[] (0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

void main()
{
    vec2 tex_offset = 1.0 / textureSize(screenTexture, 0);
    vec3 result = texture(screenTexture, TexCoords).rgb * weight[0];
    if(horizontal)
    {
        for(int i = 0; i < 5; i++)
        {
            result += texture(screenTexture, TexCoords + vec2(tex_offset.x * i, 0.0)).rgb * weight[i];
            result += texture(screenTexture, TexCoords - vec2(tex_offset.x * i, 0.0)).rgb * weight[i];
        }
    }
    else
    {
        for(int i = 0; i < 5; i++)
        {
            result += texture(screenTexture, TexCoords + vec2(0.0, tex_offset.x * i)).rgb * weight[i];
            result += texture(screenTexture, TexCoords - vec2(0.0, tex_offset.x * i)).rgb * weight[i];
        }
    }
    FragColor = vec4(result, 1.0);

})GLSL";
#pragma endregion

demo_full::demo_full(GL::cache& GLCache, GL::debug& GLDebug, const platform_io& IO)
    : GLDebug(GLDebug), TavernScene(GLCache), asteroid(GLCache)
{
    AspectRatio = (float)IO.WindowWidth / (float)IO.WindowHeight;

    // Create shader
    {
        // Assemble fragment shader strings (defines + code)
        char FragmentShaderConfig[] = "#define LIGHT_COUNT %d\n";
        snprintf(FragmentShaderConfig, ARRAY_SIZE(FragmentShaderConfig), "#define LIGHT_COUNT %d\n", TavernScene.LightCount);
        const char* FragmentShaderStrs[2] = {
            FragmentShaderConfig,
            gFragmentShaderStr,
        };
        const char* InstFragmentShaderStrs[2] = {
            FragmentShaderConfig,
            gInstancingFragmentShaderStr,
        };
        const char* ReflectFragmentShaderStrs[2] = {
            FragmentShaderConfig,
            gReflectionFragmentShaderStr,
        };

        Program =               GL::CreateProgramEx(1, &gVertexShaderStr, 2, FragmentShaderStrs, true);
        SkyProgram =            GL::CreateProgram(gVertexShaderCubeStr, gFragmentShaderCubeStr, false);
        ReflectiveProgram =     GL::CreateProgramEx(1, &gVertexShaderStr, 2, ReflectFragmentShaderStrs, true);
        InstancingProgram =     GL::CreateProgramEx(1, &gInstancingVertexShaderStr, 2, InstFragmentShaderStrs, true);
        PostProcessProgram =    GL::CreateProgram(gHdrVertexShaderStr, gHdrFragmentShaderStr, false);
        BlurProgram =           GL::CreateProgram(gHdrVertexShaderStr, BlurFragmentShaderStr, false);

    }

    // Create a vertex array and bind attribs onto the vertex buffer
    {
        glGenVertexArrays(1, &VAO);
        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, TavernScene.MeshBuffer);

        vertex_descriptor& Desc = TavernScene.MeshDesc;
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, Desc.Stride, (void*)(size_t)Desc.PositionOffset);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, Desc.Stride, (void*)(size_t)Desc.UVOffset);
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, Desc.Stride, (void*)(size_t)Desc.NormalOffset);
    }

    // Create a quad Vertex Object for FBO
    {
        float quadVertices[] = {
            // positions        // texture Coords
            -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
             1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
             1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };
        // setup plane VAO
        GLuint quadVBO = 0;
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);

        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);

        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }

    // Create sphere vertex array
    {
        int VertexMeshCount = 0;
        vertex_descriptor sphere;
        GLuint buffer = GLCache.LoadObj("media/sphere.obj", 1.f, &VertexMeshCount, &sphere);

        glGenVertexArrays(1, &SphereVAO);
        glBindVertexArray(SphereVAO);

        glBindBuffer(GL_ARRAY_BUFFER, buffer);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sphere.Stride, (void*)(size_t)sphere.PositionOffset);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sphere.Stride, (void*)(size_t)sphere.UVOffset);
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sphere.Stride, (void*)(size_t)sphere.NormalOffset);
    }

    // Create a cube vertex object for Skybox
    {
        float skyboxVertices[] = {
            // positions          
            -1.0f,  1.0f, -1.0f,
            -1.0f, -1.0f, -1.0f,
             1.0f, -1.0f, -1.0f,
             1.0f, -1.0f, -1.0f,
             1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

             1.0f, -1.0f, -1.0f,
             1.0f, -1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,
             1.0f,  1.0f, -1.0f,
             1.0f, -1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,
             1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            -1.0f,  1.0f, -1.0f,
             1.0f,  1.0f, -1.0f,
             1.0f,  1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
             1.0f, -1.0f, -1.0f,
             1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
             1.0f, -1.0f,  1.0f
        };

        GLuint SkyBuffer = 0;
        glGenVertexArrays(1, &SkyVAO);
        glGenBuffers(1, &SkyBuffer);

        glBindVertexArray(SkyVAO);
        glBindBuffer(GL_ARRAY_BUFFER, SkyBuffer);

        glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices[0], GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, (void*)0);
        glBindVertexArray(0);
    }

    // Generate skybox
    {
        std::vector<std::string> faces
        {
            "media/right.jpg",
            "media/left.jpg",
            "media/top.jpg",
            "media/bottom.jpg",
            "media/front.jpg",
            "media/back.jpg"
        };

        glGenTextures(1, &SkyTexture);
        glBindTexture(GL_TEXTURE_CUBE_MAP, SkyTexture);

        int width, height, nrChannels;
        unsigned char* data;
        for (unsigned int i = 0; i < faces.size(); i++)
        {
            unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
            if (data)
            {
                glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                    0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data
                );
            }
            stbi_image_free(data);

            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        }
        glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

        GenCubemap(EnvironmentTexture, 128.f, 128.f, GL_RGB, GL_UNSIGNED_BYTE);

        glGenFramebuffers(1, &SkyFBO);
        glBindFramebuffer(GL_FRAMEBUFFER, SkyFBO);

        GLuint depth = 0;
        glGenRenderbuffers(1, &depth);
        glBindRenderbuffer(GL_RENDERBUFFER, depth);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 128, 128);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth);

    }

    // Set initial uniforms
    {
        glUseProgram(BlurProgram);
        glUniform1i(glGetUniformLocation(BlurProgram, "screenTexture"), 0);

        glUseProgram(PostProcessProgram);
        glUniform1i(glGetUniformLocation(PostProcessProgram, "uScreenBuffer"), 0);
        glUniform1i(glGetUniformLocation(PostProcessProgram, "uBloomTexture"), 1);

        glUseProgram(InstancingProgram);
        glUniform1i(glGetUniformLocation(InstancingProgram, "uDiffuseTexture"), 0);
        glUniformBlockBinding(InstancingProgram, glGetUniformBlockIndex(InstancingProgram, "uLightBlock"), LIGHT_BLOCK_BINDING_POINT);

        glUseProgram(Program);
        glUniform1i(glGetUniformLocation(Program, "uDiffuseTexture"), 0);
        glUniform1i(glGetUniformLocation(Program, "uEmissiveTexture"), 1);
        glUniformBlockBinding(Program, glGetUniformBlockIndex(Program, "uLightBlock"), LIGHT_BLOCK_BINDING_POINT);
    }

    // Hdr floating point framebuffer
    {
        glGenFramebuffers(1, &FBO);
        glBindFramebuffer(GL_FRAMEBUFFER, FBO);

        // Create color buffer

        // set up floating point framebuffer to render scene to
        GLuint colorBuffers[2];
        glGenTextures(2, colorBuffers);
        for (unsigned int i = 0; i < 2; i++)
        {
            glBindTexture(GL_TEXTURE_2D, colorBuffers[i]);
            glTexImage2D(
                GL_TEXTURE_2D, 0, GL_RGBA16F, IO.ScreenWidth, IO.ScreenHeight, 0, GL_RGBA, GL_FLOAT, NULL
            );
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            // attach texture to framebuffer
            glFramebufferTexture2D(
                GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, colorBuffers[i], 0
            );
        }
        CBO = colorBuffers[0];
        bloomCBO = colorBuffers[1];

        unsigned int attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
        glDrawBuffers(2, attachments);

        // create render buffers instead of depth buffer
        glGenRenderbuffers(1, &RBO);
        glBindRenderbuffer(GL_RENDERBUFFER, RBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, IO.ScreenWidth, IO.ScreenHeight);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);

        // Attach buffers
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, RBO);

        // Check buffer
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cout << "Framebuffer not complete." << std::endl;
        // Unbind
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    // Blur framebuffers
    {
        glGenFramebuffers(2, pingpongFBO);
        glGenTextures(2, pingpongCBO);
        for (unsigned int i = 0; i < 2; i++)
        {
            glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[i]);
            glBindTexture(GL_TEXTURE_2D, pingpongCBO[i]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, IO.ScreenWidth, IO.ScreenHeight, 0, GL_RGBA, GL_FLOAT, NULL);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pingpongCBO[i], 0);
        }
    }
    
    // Instancing
    GenInstanceMatrices();
    RenderEnvironmentMap();
}

demo_full::~demo_full()
{
    // Cleanup GL
    glDeleteVertexArrays(1, &VAO);
    glDeleteVertexArrays(1, &quadVAO);
    glDeleteVertexArrays(1, &SphereVAO);
    glDeleteProgram(Program);
    glDeleteProgram(ReflectiveProgram);
    glDeleteProgram(SkyProgram);
    glDeleteProgram(PostProcessProgram);
    glDeleteProgram(BlurProgram);
    glDeleteProgram(InstancingProgram);
    glDeleteFramebuffers(2, pingpongFBO);
    glDeleteFramebuffers(1, &FBO);
    glDeleteFramebuffers(1, &SkyFBO);
    glDeleteRenderbuffers(1, &RBO);
}

void demo_full::Update(const platform_io& IO)
{
    AspectRatio = (float)IO.WindowWidth / (float)IO.WindowHeight;

    Camera = CameraUpdateFreefly(Camera, IO.CameraInputs);

#pragma region Draw scene in FBO

    RenderEnvironmentMap();

    glViewport(0, 0, IO.WindowWidth, IO.WindowHeight);

    RenderScene(Camera);

#pragma endregion

#pragma region Blur Process
    bool horizontal = true, first_iteration = true;
    if (processBloom)
    {
        glUseProgram(BlurProgram);
        for (int i = 0; i < pingpongAmount; i++)
        {
            glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[horizontal]);
            glUniform1i(glGetUniformLocation(BlurProgram, "horizontal"), horizontal);

            if (first_iteration)
            {
                glBindTexture(GL_TEXTURE_2D, bloomCBO);
                first_iteration = false;
            }
            else
            {
                glBindTexture(GL_TEXTURE_2D, pingpongCBO[!horizontal]);
            }

            glDisable(GL_DEPTH_TEST);
            RenderQuad();

            horizontal = !horizontal;
        }
    }
#pragma endregion

#pragma region Draw post-process HDR

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(PostProcessProgram);
    // Set uniforms
    glUniform1i(glGetUniformLocation(PostProcessProgram, "uProcessHdr"), processHdr);
    glUniform1i(glGetUniformLocation(PostProcessProgram, "uProcessGamma"), processGamma);
    glUniform1i(glGetUniformLocation(PostProcessProgram, "uProcessBloom"), processBloom);
    glUniform1f(glGetUniformLocation(PostProcessProgram, "uGamma"), gamma);
    glUniform1f(glGetUniformLocation(PostProcessProgram, "uExposure"), exposure);

    glUniform1i(glGetUniformLocation(PostProcessProgram, "uProcessInverse"), processInverse);
    glUniform1i(glGetUniformLocation(PostProcessProgram, "uProcessGreyScale"), processGreyScale);
    glUniform1i(glGetUniformLocation(PostProcessProgram, "uProcessKernel"), processKernel);
    glUniformMatrix3fv(glGetUniformLocation(PostProcessProgram, "uKernel"), 1, GL_FALSE, kernelMat.e);
    glUniform1f(glGetUniformLocation(PostProcessProgram, "x_ratio"), x_ratio_kernel);
    glUniform1f(glGetUniformLocation(PostProcessProgram, "y_ratio"), y_ratio_kernel);

    glDisable(GL_DEPTH_TEST);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, CBO);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, pingpongCBO[!horizontal]);

    RenderQuad();

#pragma endregion

    // Display debug UI
    this->DisplayDebugUI();
}

void demo_full::DisplayDebugUI()
{
    ImGui::Checkbox("Wireframe", &Wireframe);
    ImGui::Spacing();

    if (ImGui::TreeNodeEx("demo_full", ImGuiTreeNodeFlags_Framed))
    {
        if (ImGui::TreeNode("Reflection"))
        {
            ImGui::Checkbox("Dynamic", &Dynamic);

            ImGui::TreePop();
        }

        ImGui::Spacing();

        if (ImGui::TreeNode("Post process"))
        {
            ImGui::Checkbox("Grey scale", &processGreyScale);
            ImGui::Checkbox("Inverse", &processInverse);
            ImGui::Checkbox("Kernel effects", &processKernel);

            if (processKernel)
            {
                const char* items[] = { "Normal", "Kernel effects 1", "Kernel effects 2" , "Kernel effects 3" , "Kernel effects 4" };
                static int current = 0;
                if (ImGui::ListBox("Post Process Type", &current, items, IM_ARRAYSIZE(items), IM_ARRAYSIZE(items)))
                {
                    switch (current)
                    {
                    case 1:
                        kernelMat = {
                            1.0 / 9, 1.0 / 9, 1.0 / 9,
                            1.0 / 9, 1.0 / 9, 1.0 / 9,
                            1.0 / 9, 1.0 / 9, 1.0 / 9
                        };
                        break;
                    case 2:
                        kernelMat = {
                            2,  2,  2,
                            2, -15,  2,
                            2,  2,  2
                        };
                        break;
                    case 3:
                        kernelMat = {
                            2,  2,  2,
                            2, -16,  2,
                            2,  2,  2
                        };
                        break;
                    case 4:
                        kernelMat = {
                            -2, -1,  0,
                            -1,  1,  1,
                             0,  1,  2
                        };
                        break;
                    default:
                        kernelMat = {
                            0,0,0,
                            0,1,0,
                            0,0,0
                        };
                        break;
                    }
                }

                ImGui::Spacing();

                ImGui::DragFloat3("c0", kernelMat.c[0].e, 0.01f);
                ImGui::DragFloat3("c1", kernelMat.c[1].e, 0.01f);
                ImGui::DragFloat3("c2", kernelMat.c[2].e, 0.01f);

                ImGui::Spacing();

                ImGui::Checkbox("Ratio X = Y", &ratioXYkernel);
                if (ratioXYkernel)
                {
                    ImGui::SliderFloat("Ratio XY", &x_ratio_kernel, 5.f, 1000.f);
                    y_ratio_kernel = x_ratio_kernel;
                }
                else
                {
                    ImGui::SliderFloat("Ratio X", &x_ratio_kernel, 5.f, 1000.f);
                    ImGui::SliderFloat("Ratio Y", &y_ratio_kernel, 5.f, 1000.f);
                }
            }
            ImGui::TreePop();
        }

        ImGui::Spacing();

        if (ImGui::TreeNode("HDR"))
        {
            ImGui::Checkbox("HDR", &processHdr);
            if (processHdr)
                ImGui::SliderFloat("Exposure", &exposure, 0.1f, 8.f);

            ImGui::Spacing();

            ImGui::Checkbox("Process gamma", &processGamma);
            if (processGamma)
                ImGui::SliderFloat("Gamma", &gamma, 0.45f, 4.4f);

            ImGui::Spacing();

            ImGui::Checkbox("Process bloom", &processBloom);
            if (processBloom)
            {
                ImGui::SliderFloat("Brightness clamp", &brightnessClamp, 0.f, 1.f);
                ImGui::SliderInt("Blur amount", &pingpongAmount, 1, 30);
            }
            ImGui::TreePop();
        }

        ImGui::Spacing();

        if (ImGui::TreeNode("Instancing"))
        {
            ImGui::Checkbox("Activate", &processInstancing);

            ImGui::Spacing();

            ImGui::DragInt("Instance count", &instanceCount);
            ImGui::DragFloat("Circle radius", &instanceCircleRadius);
            ImGui::DragFloat("Offset", &instanceOffset);

            ImGui::TreePop();
        }

        ImGui::Spacing();

        if (ImGui::TreeNodeEx("Camera"))
        {
            ImGui::Text("Position: (%.2f, %.2f, %.2f)", Camera.Position.x, Camera.Position.y, Camera.Position.z);
            ImGui::Text("Pitch: %.2f", Math::ToDegrees(Camera.Pitch));
            ImGui::Text("Yaw: %.2f", Math::ToDegrees(Camera.Yaw));
            ImGui::TreePop();
        }
        TavernScene.InspectLights();

        ImGui::TreePop();
    }
}

void demo_full::GenCubemap(GLuint& index, const float width, const float height, const GLint format, const GLint size)
{
    glGenTextures(1, &index);
    glBindTexture(GL_TEXTURE_CUBE_MAP, index);

    for (unsigned int i = 0; i < 6; i++)
    {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, format, width, height, 0, format, size, NULL);

        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    }
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

void demo_full::GenInstanceMatrices()
{
    static int previousCount = 0;
    static float previousOffset = 0.f;
    static std::vector<v2> speeds;
    static std::vector<v2> transforms;
    static std::vector<v3> displacements;

    std::vector<mat4> modelMatrices;
    modelMatrices.resize(instanceCount);

    bool newRange = previousCount != instanceCount || previousOffset != instanceOffset;
    previousCount = instanceCount;
    previousOffset = instanceOffset;

    if (newRange)
    {
        speeds.resize(instanceCount);
        displacements.resize(instanceCount);
        transforms.resize(instanceCount);
    }

    srand((unsigned int)ImGui::GetTime());

    auto displace = [](float offset)->float { return (rand() % (int)(offset * 100.f)) / 100.f - offset;  };

    for (int i = 0; i < instanceCount; i++)
    {
        if (newRange)
        {
            speeds[i] = { 0.f, (float)(rand() % 100) / 10000.f };
            float offset = instanceOffset == 0.f ? 1.f : instanceOffset;
            displacements[i] = { displace(offset) , displace(offset) , displace(offset) };
            transforms[i] = { (rand() % 20) / 100.0f + 0.05f , (float)(rand() % 360) };
        }

        speeds[i].x += speeds[i].y;

        mat4 model = Mat4::Identity();
        // position
        float angle = (float)i / (float)instanceCount * 360.f + speeds[i].x;
        float x = sin(angle) * instanceCircleRadius + displacements[i].x;
        float y = displacements[i].y * 0.4f;
        float z = cos(angle) * instanceCircleRadius + displacements[i].z;
        model = Mat4::Translate(v3{ x, y, z });

        // scale
        float scale = transforms[i].x;
        model = model * Mat4::Scale(v3{ scale, scale, scale });

        //rotation
        float rotation = transforms[i].y;
        model = model * Mat4::RotateX(rotation);

        modelMatrices[i] = model;
    }

    GenInstanceVBO(modelMatrices);
}

void demo_full::GenInstanceVBO(const std::vector<mat4>& modelMatrices)
{
    glBindVertexArray(asteroid.VAO);

    GLuint instanceVBO = 0;
    glGenBuffers(1, &instanceVBO);
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);

    glBufferData(GL_ARRAY_BUFFER, instanceCount * sizeof(mat4), &modelMatrices.data()[0], GL_STATIC_DRAW);

    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(mat4), (void*)0);
    glVertexAttribDivisor(3, 1);

    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(mat4), (void*)(sizeof(v4)));
    glVertexAttribDivisor(4, 1);

    glEnableVertexAttribArray(5);
    glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(mat4), (void*)(2 * sizeof(v4)));
    glVertexAttribDivisor(5, 1);

    glEnableVertexAttribArray(6);
    glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(mat4), (void*)(3 * sizeof(v4)));
    glVertexAttribDivisor(6, 1);

    glBindVertexArray(0);
}

void demo_full::RenderEnvironmentMap()
//Update environment skybox 
{
    // generate an FBO 
    glBindFramebuffer(GL_FRAMEBUFFER, SkyFBO);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);

    glViewport(0, 0, 128, 128);
    // render the scene then push the fbo in
    for (int i = 0; i < 6; i++)
    {
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, EnvironmentTexture, 0);
        // switch between the 6 faces of the cubemap
        RenderingCamera.SetFace(i);
        RenderScene(RenderingCamera, false);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void demo_full::RenderScene(const camera& cam, bool reflection)
{
    // Bind only if called with reflection
    if (reflection)
        glBindFramebuffer(GL_FRAMEBUFFER, FBO);

    // Clear screen
    glClearColor(0.f, 0.f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    mat4 ProjectionMatrix = Mat4::Perspective(Math::ToRadians(60.f), AspectRatio, 0.1f, 100.f);
    mat4 ViewMatrix = CameraGetInverseMatrix(cam);
    mat4 ModelMatrix = Mat4::Translate({ 0.f, 0.f, 0.f });

    RenderSkybox(cam, ProjectionMatrix);
    RenderTavern(ProjectionMatrix, ViewMatrix, ModelMatrix);

    if (processInstancing)
        RenderAsteroids(ProjectionMatrix, ViewMatrix, ModelMatrix);

    if (reflection)
        RenderReflectiveSphere(ProjectionMatrix, ViewMatrix, ModelMatrix);

    // Render tavern wireframe
    if (Wireframe)
    {
        GLDebug.Wireframe.BindBuffer(TavernScene.MeshBuffer, TavernScene.MeshDesc.Stride, TavernScene.MeshDesc.PositionOffset, TavernScene.MeshVertexCount);
        GLDebug.Wireframe.DrawArray(0, TavernScene.MeshVertexCount, ProjectionMatrix * ViewMatrix * ModelMatrix);
    }
}

void demo_full::RenderSkybox(const camera& cam, const mat4& projection)
{
    glDepthMask(GL_FALSE);
    glUseProgram(SkyProgram);
    mat4 ViewMatrixWT = CameraGetInverseMatrixWT(cam);

    glUniformMatrix4fv(glGetUniformLocation(SkyProgram, "projection"), 1, GL_FALSE, projection.e);
    glUniformMatrix4fv(glGetUniformLocation(SkyProgram, "view"), 1, GL_FALSE, ViewMatrixWT.e);

    glBindVertexArray(SkyVAO);
    glBindTexture(GL_TEXTURE_CUBE_MAP, SkyTexture);
    glDrawArrays(GL_TRIANGLES, 0, 36);

    glDepthMask(GL_TRUE);
    glBindVertexArray(0);
}

void demo_full::RenderQuad()
{
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

void demo_full::RenderTavern(const mat4& ProjectionMatrix, const mat4& ViewMatrix, const mat4& ModelMatrix)
{
    glEnable(GL_DEPTH_TEST);

    // Use shader and configure its uniforms
    glUseProgram(Program);

    // Set uniforms
    mat4 NormalMatrix = Mat4::Transpose(Mat4::Inverse(ModelMatrix));
    glUniformMatrix4fv(glGetUniformLocation(Program, "uProjection"), 1, GL_FALSE, ProjectionMatrix.e);
    glUniformMatrix4fv(glGetUniformLocation(Program, "uModel"), 1, GL_FALSE, ModelMatrix.e);
    glUniformMatrix4fv(glGetUniformLocation(Program, "uView"), 1, GL_FALSE, ViewMatrix.e);
    glUniformMatrix4fv(glGetUniformLocation(Program, "uModelNormalMatrix"), 1, GL_FALSE, NormalMatrix.e);
    glUniform3fv(glGetUniformLocation(Program, "uViewPosition"), 1, Camera.Position.e);

    glUniform1f(glGetUniformLocation(Program, "uBrightness"), brightnessClamp);

    // Bind uniform buffer and textures
    glBindBufferBase(GL_UNIFORM_BUFFER, LIGHT_BLOCK_BINDING_POINT, TavernScene.LightsUniformBuffer);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, TavernScene.LinearDiffuseTexture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, TavernScene.EmissiveTexture);

    glActiveTexture(GL_TEXTURE0); // Reset active texture just in case

    // Draw mesh
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, TavernScene.MeshVertexCount);
}

void demo_full::RenderAsteroids(const mat4& ProjectionMatrix, const mat4& ViewMatrix, const mat4& ModelMatrix)
{
    glEnable(GL_DEPTH_TEST);

    // Use shader and configure its uniforms
    glUseProgram(InstancingProgram);

    // Set uniforms
    mat4 NormalMatrix = Mat4::Transpose(Mat4::Inverse(ModelMatrix));
    glUniformMatrix4fv(glGetUniformLocation(InstancingProgram, "uProjection"), 1, GL_FALSE, ProjectionMatrix.e);
    glUniformMatrix4fv(glGetUniformLocation(InstancingProgram, "uView"), 1, GL_FALSE, ViewMatrix.e);
    glUniformMatrix4fv(glGetUniformLocation(InstancingProgram, "uModelNormalMatrix"), 1, GL_FALSE, NormalMatrix.e);
    glUniform3fv(glGetUniformLocation(InstancingProgram, "uViewPosition"), 1, Camera.Position.e);

    // Bind uniform buffer and textures
    glBindBufferBase(GL_UNIFORM_BUFFER, LIGHT_BLOCK_BINDING_POINT, TavernScene.LightsUniformBuffer);
    glBindTexture(GL_TEXTURE_2D, asteroid.DiffuseTexture);

    GenInstanceMatrices();

    asteroid.Draw(instanceCount);
}

void demo_full::RenderReflectiveSphere(const mat4& ProjectionMatrix, const mat4& ViewMatrix, const mat4& ModelMatrix)
{
    glUseProgram(ReflectiveProgram);

    // Render Sphere
    mat4 model = Mat4::Translate({ -4.f, 0.f, 0.f }) * Mat4::Scale({1.5f, 1.5f, 1.5f});
    mat4 NormalMatrix = Mat4::Transpose(Mat4::Inverse(model));
    glUniformMatrix4fv(glGetUniformLocation(ReflectiveProgram, "uProjection"), 1, GL_FALSE, ProjectionMatrix.e);
    glUniformMatrix4fv(glGetUniformLocation(ReflectiveProgram, "uModel"), 1, GL_FALSE, model.e);
    glUniformMatrix4fv(glGetUniformLocation(ReflectiveProgram, "uView"), 1, GL_FALSE, ViewMatrix.e);
    glUniformMatrix4fv(glGetUniformLocation(ReflectiveProgram, "uModelNormalMatrix"), 1, GL_FALSE, NormalMatrix.e);
    glUniform3fv(glGetUniformLocation(ReflectiveProgram, "uViewPosition"), 1, Camera.Position.e);

    glBindVertexArray(SphereVAO);
    if (Dynamic)
        glBindTexture(GL_TEXTURE_CUBE_MAP, EnvironmentTexture);
    else
        glBindTexture(GL_TEXTURE_CUBE_MAP, SkyTexture);

    glDrawArrays(GL_TRIANGLES, 0, 2880);
}

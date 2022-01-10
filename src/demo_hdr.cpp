
#include <vector>
#include <iostream>

#include <imgui.h>

#include "opengl_helpers.h"
#include "opengl_helpers_wireframe.h"

#include "color.h"
#include "maths.h"
#include "mesh.h"

#include "demo_hdr.h"

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
uniform bool uGamma;
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

    // Apply gamma correction
    if (uGamma)
    {
        float gamma = 2.2;
        oColor.rgb = pow(oColor.rgb, vec3(1.0/gamma));
    }
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

uniform bool uProcess;
uniform bool uGamma;
uniform float uExposure;

// shader ouputs
out vec4 FragColor;

void main()
{
	const float gamma = 2.2;
	vec3 hdrColor = texture(uScreenTexture, TexCoords).rgb;
    vec3 bloomColor = texture(uBloomTexture, TexCoords).rgb;

    hdrColor = hdrColor + bloomColor;

	if (uProcess)
	{
		vec3 toneMapped = vec3(1.0) - exp(-hdrColor * uExposure);
        hdrColor = toneMapped;
	}
	if (uGamma)
	{
		hdrColor = pow(hdrColor, vec3(1.0/gamma));
	}
    
    FragColor = vec4(hdrColor, 1.0);
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

demo_hdr::demo_hdr(GL::cache& GLCache, GL::debug& GLDebug, const platform_io& IO)
    : GLDebug(GLDebug), TavernScene(GLCache)
{
    if (processGamma)
        glEnable(GL_FRAMEBUFFER_SRGB);

    // Create shader
    {
        // Assemble fragment shader strings (defines + code)
        char FragmentShaderConfig[] = "#define LIGHT_COUNT %d\n";
        snprintf(FragmentShaderConfig, ARRAY_SIZE(FragmentShaderConfig), "#define LIGHT_COUNT %d\n", TavernScene.LightCount);
        const char* FragmentShaderStrs[2] = {
            FragmentShaderConfig,
            gFragmentShaderStr,
        };

        Program = GL::CreateProgramEx(1, &gVertexShaderStr, 2, FragmentShaderStrs, true);

        hdrProgram = GL::CreateProgram(gHdrVertexShaderStr, gHdrFragmentShaderStr, false);

        blurProgram = GL::CreateProgram(gHdrVertexShaderStr, BlurFragmentShaderStr, false);
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

    // Set initial uniforms
    {
        glUseProgram(blurProgram);
        glUniform1i(glGetUniformLocation(blurProgram, "screenTexture"), 0);
        
        glUseProgram(hdrProgram);
        glUniform1i(glGetUniformLocation(hdrProgram, "uScreenBuffer"), 0);
        glUniform1i(glGetUniformLocation(hdrProgram, "uBloomTexture"), 1);

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
        hdrCBO = colorBuffers[0];
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
}

demo_hdr::~demo_hdr()
{
    // Cleanup GL
    glDeleteVertexArrays(1, &VAO);
    glDeleteVertexArrays(1, &quadVAO);
    glDeleteProgram(Program);
    glDeleteProgram(hdrProgram);
}

void demo_hdr::Update(const platform_io& IO)
{

    const float AspectRatio = (float)IO.WindowWidth / (float)IO.WindowHeight;
    glViewport(0, 0, IO.WindowWidth, IO.WindowHeight);
    
    Camera = CameraUpdateFreefly(Camera, IO.CameraInputs);


#pragma region Draw scene in FBO
    glBindFramebuffer(GL_FRAMEBUFFER, FBO);

    // Clear screen
    glClearColor(0.f, 0.f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    mat4 ProjectionMatrix = Mat4::Perspective(Math::ToRadians(60.f), AspectRatio, 0.1f, 100.f);
    mat4 ViewMatrix = CameraGetInverseMatrix(Camera);
    mat4 ModelMatrix = Mat4::Translate({ 0.f, 0.f, 0.f });

    // Render tavern
    this->RenderTavern(ProjectionMatrix, ViewMatrix, ModelMatrix);

#pragma endregion

#pragma region Blur Process
    bool horizontal = true, first_iteration = true;
    glUseProgram(blurProgram);
    for (unsigned int i = 0; i < pingpongAmount; i++)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[horizontal]);
        glUniform1i(glGetUniformLocation(blurProgram, "horizontal"), horizontal);

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
#pragma endregion

#pragma region Draw post-process HDR
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(hdrProgram);
    // Set uniforms
    glUniform1i(glGetUniformLocation(hdrProgram, "uProcess"), processHdr);
    glUniform1i(glGetUniformLocation(hdrProgram, "uGamma"), false);
    glUniform1f(glGetUniformLocation(hdrProgram, "uExposure"), exposure);

    glDisable(GL_DEPTH_TEST);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, hdrCBO);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, pingpongCBO[!horizontal]);
        
    RenderQuad();
#pragma endregion
    
    // Render tavern wireframe
    if (Wireframe)
    {
        GLDebug.Wireframe.BindBuffer(TavernScene.MeshBuffer, TavernScene.MeshDesc.Stride, TavernScene.MeshDesc.PositionOffset, TavernScene.MeshVertexCount);
        GLDebug.Wireframe.DrawArray(0, TavernScene.MeshVertexCount, ProjectionMatrix * ViewMatrix * ModelMatrix);
    }
    // Display debug UI
    this->DisplayDebugUI();
}

void demo_hdr::DisplayDebugUI()
{
    if (ImGui::TreeNodeEx("demo_hdr", ImGuiTreeNodeFlags_Framed))
    {
        // Debug display
        ImGui::Checkbox("HDR", &processHdr);
        if (ImGui::Checkbox("Gamma", &processGamma));
            processGamma ? glEnable(GL_FRAMEBUFFER_SRGB) : glDisable(GL_FRAMEBUFFER_SRGB);

        ImGui::SliderFloat("Exposure", &exposure, 0.1f, 8.f);
        ImGui::SliderFloat("Brightness clamp", &brightnessClamp, 0.f, 1.f);
        ImGui::SliderInt("Blur amount", &pingpongAmount, 1, 20);

        ImGui::Spacing();

        ImGui::Checkbox("Wireframe", &Wireframe);
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

void demo_hdr::RenderQuad()
{
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

void demo_hdr::RenderTavern(const mat4& ProjectionMatrix, const mat4& ViewMatrix, const mat4& ModelMatrix)
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
    
    // Gamma correction
    //glUniform1i(glGetUniformLocation(Program, "uGamma"), processGamma);
    glUniform1f(glGetUniformLocation(Program, "uBrightness"), brightnessClamp);

    // Bind uniform buffer and textures
    glBindBufferBase(GL_UNIFORM_BUFFER, LIGHT_BLOCK_BINDING_POINT, TavernScene.LightsUniformBuffer);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, TavernScene.DiffuseTexture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, TavernScene.EmissiveTexture);

    glActiveTexture(GL_TEXTURE0); // Reset active texture just in case
    
    // Draw mesh
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, TavernScene.MeshVertexCount);
}

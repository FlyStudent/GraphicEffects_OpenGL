
#include <vector>

#include <imgui.h>

#include "opengl_helpers.h"
#include "opengl_helpers_wireframe.h"

#include "color.h"
#include "maths.h"
#include "mesh.h"

#include "demo_skybox.h"

#include <iostream>
#include "stb_image.h"

const int LIGHT_BLOCK_BINDING_POINT = 0;

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

static const char* gFragmentShaderStr = R"GLSL(
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


static const char* gFragmentShaderCubeStr = R"GLSL(
out vec4 FragColor;

in vec3 TexCoords;

uniform samplerCube skybox;

void main()
{    
    FragColor = texture(skybox, TexCoords);
})GLSL";

demo_skybox::demo_skybox(GL::cache& GLCache, GL::debug& GLDebug)
    : demo_base(GLCache, GLDebug), GLDebug(GLDebug)
{
    // Create shaders
    {
        // Assemble fragment shader strings (defines + code)
        char FragmentShaderConfig[] = "#define LIGHT_COUNT %d\n";
        snprintf(FragmentShaderConfig, ARRAY_SIZE(FragmentShaderConfig), "#define LIGHT_COUNT %d\n", TavernScene.LightCount);
        const char* FragmentShaderStrs[2] = {
            FragmentShaderConfig,
            gFragmentShaderStr,
        };

        //this->Program = 

        this->SkyProgram = GL::CreateProgram(gVertexShaderCubeStr, gFragmentShaderCubeStr);
        this->ReflectiveProgram = GL::CreateProgramEx(1, &gVertexShaderStr, 2, FragmentShaderStrs, true);
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

    // Create sphere vertex array
    {
        int VertexMeshCount = 0;
        GLuint buffer = GLCache.LoadObj("media/sphere.obj",1.f, &VertexMeshCount);
        vertex_descriptor sphere;

        sphere.Stride = sizeof(vertex_full);
        sphere.HasNormal = true;
        sphere.HasUV = true;
        sphere.PositionOffset = OFFSETOF(vertex_full, Position);
        sphere.UVOffset = OFFSETOF(vertex_full, UV);
        sphere.NormalOffset = OFFSETOF(vertex_full, Normal);

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

    // Create a vertex array and bind attribs onto the vertex buffer
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

        glGenVertexArrays(1, &SkyVAO);
        glGenBuffers(1, &SkyBuffer);
       
        glBindVertexArray(SkyVAO);
        glBindBuffer(GL_ARRAY_BUFFER, SkyBuffer);
        
        glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices[0], GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, (void*)0);
        glBindVertexArray(0);

    }

    // Set uniforms that won't change
    {
        glUseProgram(Program);
        glUniform1i(glGetUniformLocation(Program, "uDiffuseTexture"), 0);
        glUniform1i(glGetUniformLocation(Program, "uEmissiveTexture"), 1);
        glUniformBlockBinding(Program, glGetUniformBlockIndex(Program, "uLightBlock"), LIGHT_BLOCK_BINDING_POINT);
    }

    //Create Skybox

    std::vector<std::string> faces
    {
        "media/Sky_NightTime01FT.png",
        "media/Sky_NightTime01BK.png",
        "media/Sky_NightTime01UP.png",
        "media/Sky_NightTime01DN.png",
        "media/Sky_NightTime01RT.png",
        "media/Sky_NightTime01LF.png",
    };

    {
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
                stbi_image_free(data);
            }
            else
            {
                std::cout << "Cubemap tex failed to load at path: " << faces[i] << std::endl;
                stbi_image_free(data);
            }

            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        }
    }
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

    // Create Empty cubemap
    {
        glGenTextures(1, &EnvironmentTexture);
        glBindTexture(GL_TEXTURE_CUBE_MAP, EnvironmentTexture);

        for (unsigned int i = 0; i < faces.size(); i++)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, 128, 128, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        }
    }

    RenderEnvironmentMap({0,0,0});
};

demo_skybox::~demo_skybox()
{
    // Cleanup GL
    glDeleteVertexArrays(1, &VAO);
    glDeleteVertexArrays(1, &SkyVAO);
    glDeleteProgram(Program);
    glDeleteProgram(ReflectiveProgram);
    glDeleteProgram(SkyProgram);
}

void demo_skybox::RenderEnvironmentMap(const v3& center) 
{
    // Set center of rendering
    RenderingCamera.Position = center;

    // generate empty cubemap 
    // generate an FBO 
    GLuint fbo = 0;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);

    GLuint depth = 0;
    glGenRenderbuffers(1, &depth);
    glBindRenderbuffer(GL_RENDERBUFFER, depth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 128, 128);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth);

    glViewport(0, 0, 128, 128);
    // render the scene then push the fbo in
    for (int i = 0; i < 6 ; i++)
    {
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, EnvironmentTexture, 0);
        // switch between the 6 faces of the cubemap
        RenderingCamera.SetFace(i);

        //render the scene in the fbo
        mat4 ProjectionMatrixUnit = Mat4::Perspective(Math::ToRadians(90.f), 1.f, 0.1f, 100.f);
        mat4 ViewMatrix = CameraGetInverseMatrix(RenderingCamera);
        mat4 ModelMatrix = Mat4::Translate({ 0.f, 0.f, 0.f });
        mat4 ViewMatrixWT = CameraGetInverseMatrixWT(Camera);
        
        glClearColor(0.f, 0.f, 0.f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        glUseProgram(SkyProgram);
        // ... set view and projection matrix

        glUniformMatrix4fv(glGetUniformLocation(SkyProgram, "projection"), 1, GL_FALSE, ProjectionMatrixUnit.e);
        glUniformMatrix4fv(glGetUniformLocation(SkyProgram, "view"), 1, GL_FALSE, ViewMatrixWT.e);

        glBindVertexArray(SkyVAO);
        glBindTexture(GL_TEXTURE_CUBE_MAP, SkyTexture);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        this->RenderTavernEnv(ProjectionMatrixUnit, ViewMatrix, ModelMatrix);
    }
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    /*glDeleteRenderbuffers(1, &depth);
    glDeleteFramebuffers(1, &fbo);
    */
}

void demo_skybox::Update(const platform_io& IO)
{
    const float AspectRatio = (float)IO.WindowWidth / (float)IO.WindowHeight;
    glViewport(0, 0, IO.WindowWidth, IO.WindowHeight);

    Camera = CameraUpdateFreefly(Camera, IO.CameraInputs);

    // Clear screen
    glClearColor(0.f, 0.f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    mat4 ProjectionMatrix = Mat4::Perspective(Math::ToRadians(60.f), AspectRatio, 0.1f, 100.f);
    mat4 ViewMatrix = CameraGetInverseMatrix(Camera);
    mat4 ModelMatrix = Mat4::Translate({ 0.f, 0.f, 0.f });

    mat4 ViewMatrixWT = CameraGetInverseMatrixWT(Camera);
 
    // Render Skybox 
    glDepthMask(GL_FALSE);
    glUseProgram(SkyProgram);
    // ... set view and projection matrix
        
    glUniformMatrix4fv(glGetUniformLocation(SkyProgram, "projection"), 1, GL_FALSE, ProjectionMatrix.e);
    glUniformMatrix4fv(glGetUniformLocation(SkyProgram, "view"), 1, GL_FALSE, ViewMatrixWT.e);

    glBindVertexArray(SkyVAO);
    glBindTexture(GL_TEXTURE_CUBE_MAP, SkyTexture);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glDepthMask(GL_TRUE);

    // Render tavern
    this->RenderTavern(ProjectionMatrix, ViewMatrix, ModelMatrix);
    
    // Render Sphere used to reflect
    glUseProgram(ReflectiveProgram);

    mat4 NormalMatrix = Mat4::Transpose(Mat4::Inverse(ModelMatrix));

    glUniformMatrix4fv(glGetUniformLocation(ReflectiveProgram, "uProjection"), 1, GL_FALSE, ProjectionMatrix.e);
    glUniformMatrix4fv(glGetUniformLocation(ReflectiveProgram, "uModel"), 1, GL_FALSE, ModelMatrix.e);
    glUniformMatrix4fv(glGetUniformLocation(ReflectiveProgram, "uView"), 1, GL_FALSE, ViewMatrix.e);
    glUniformMatrix4fv(glGetUniformLocation(ReflectiveProgram, "uModelNormalMatrix"), 1, GL_FALSE, NormalMatrix.e);
    glUniform3fv(glGetUniformLocation(ReflectiveProgram, "uViewPosition"), 1, Camera.Position.e);

    glBindVertexArray(SphereVAO);
    if (Dynamic)
        glBindTexture(GL_TEXTURE_CUBE_MAP, EnvironmentTexture);
    else 
        glBindTexture(GL_TEXTURE_CUBE_MAP, SkyTexture);

    glDrawArrays(GL_TRIANGLES, 0, 2880);

    // Render tavern wireframe
    if (Wireframe)
    {
        GLDebug.Wireframe.BindBuffer(TavernScene.MeshBuffer, TavernScene.MeshDesc.Stride, TavernScene.MeshDesc.PositionOffset, TavernScene.MeshVertexCount);
        GLDebug.Wireframe.DrawArray(0, TavernScene.MeshVertexCount, ProjectionMatrix * ViewMatrix * ModelMatrix);
    }

    // Display debug UI
    this->DisplayDebugUI();
}

void demo_skybox::DisplayDebugUI()
{
    if (ImGui::TreeNodeEx("demo_Skybox", ImGuiTreeNodeFlags_Framed))
    {
        // Debug display
        ImGui::Checkbox("Wireframe", &Wireframe);
        ImGui::Checkbox("Dynamic Reflection", &Dynamic);
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

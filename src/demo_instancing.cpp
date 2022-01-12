
#include <vector>

#include <imgui.h>

#include "opengl_helpers.h"
#include "opengl_helpers_wireframe.h"

#include "color.h"
#include "maths.h"
#include "mesh.h"

#include "demo_instancing.h"


const int LIGHT_BLOCK_BINDING_POINT = 0;

#pragma region BASIC VS
static const char* gVertexShaderStr = R"GLSL(
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

#pragma region BASIC FS
static const char* gFragmentShaderStr = R"GLSL(

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

asteroid_mesh::asteroid_mesh(GL::cache& GLCache)
{
    // Create mesh
    {
        // Use vbo from GLCache
        VBO = GLCache.LoadObj("media/rock.obj", 1.f, &MeshVertexCount);

        MeshDesc.Stride = sizeof(vertex_full);
        MeshDesc.HasNormal = true;
        MeshDesc.HasUV = true;
        MeshDesc.PositionOffset = OFFSETOF(vertex_full, Position);
        MeshDesc.UVOffset = OFFSETOF(vertex_full, UV);
        MeshDesc.NormalOffset = OFFSETOF(vertex_full, Normal);

        glGenVertexArrays(1, &VAO);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, MeshDesc.Stride, (void*)(size_t)MeshDesc.PositionOffset);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, MeshDesc.Stride, (void*)(size_t)MeshDesc.UVOffset);
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, MeshDesc.Stride, (void*)(size_t)MeshDesc.NormalOffset);
    }

    // Gen texture
    {
        DiffuseTexture = GLCache.LoadTexture("media/rock.png", IMG_FLIP | IMG_GEN_MIPMAPS);
    }
}

void asteroid_mesh::Draw(int instance)
{
    glBindTexture(GL_TEXTURE_2D, DiffuseTexture);
    // Draw mesh
    glBindVertexArray(VAO);
    glDrawArraysInstanced(GL_TRIANGLES, 0, MeshVertexCount, instance);
    //glDrawElementsInstanced(GL_TRIANGLES, MeshVertexCount, GL_UNSIGNED_INT, 0, INSTANCE);
}


demo_instancing::demo_instancing(GL::cache& GLCache, GL::debug& GLDebug)
    : GLDebug(GLDebug), asteroid(GLCache)
{
    Lights.resize(LightCount);

    // Create shader
    {
        // Assemble fragment shader strings (defines + code)
        char FragmentShaderConfig[] = "#define LIGHT_COUNT %d\n";
        snprintf(FragmentShaderConfig, ARRAY_SIZE(FragmentShaderConfig), "#define LIGHT_COUNT %d\n", LightCount);
        const char* FragmentShaderStrs[2] = {
            FragmentShaderConfig,
            gFragmentShaderStr,
        };

        this->Program = GL::CreateProgramEx(1, &gVertexShaderStr, 2, FragmentShaderStrs, true);
    }


    // (Default light, standard values)
    {
        GL::light DefaultLight = {};
        DefaultLight.Enabled = true;
        DefaultLight.Position = { 0.0f, 0.0f, 1.0f, 4.f };
        DefaultLight.Ambient = { 0.2f, 0.2f, 0.2f };
        DefaultLight.Diffuse = { 1.0f, 1.0f, 1.0f };
        DefaultLight.Specular = { 0.0f, 0.0f, 0.0f };
        DefaultLight.Attenuation = { 1.0f, 0.0f, 0.0f };

        // Sun light
        this->Lights[0] = DefaultLight;
    }

    Texture = GLCache.LoadTexture("media/brick.png", IMG_FLIP | IMG_GEN_MIPMAPS);

    // Create a quad Vertex Object
    {
        float quadVertices[] = {
            // positions        // UVs      // normals  
            -1.0f,  1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
             1.0f,  1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
             1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
             1.0f,  1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f,
        };
        // setup plane VAO
        GLuint quadVBO = 0;
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);

        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);

        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));
    }

    // Set uniforms that won't change
    {
        glUseProgram(Program);
        glUniform1i(glGetUniformLocation(Program, "uDiffuseTexture"), 0);
        //glUniform1i(glGetUniformLocation(Program, "uEmissiveTexture"), 1);
        glUniformBlockBinding(Program, glGetUniformBlockIndex(Program, "uLightBlock"), LIGHT_BLOCK_BINDING_POINT);
    }

    // Gen light uniform buffer
    {
        glGenBuffers(1, &LightsUniformBuffer);
        glBindBuffer(GL_UNIFORM_BUFFER, LightsUniformBuffer);
        glBufferData(GL_UNIFORM_BUFFER, LightCount * sizeof(GL::light), Lights.data(), GL_DYNAMIC_DRAW);
    }

    GenMatrices();
}

demo_instancing::~demo_instancing()
{
    // Cleanup GL
    glDeleteVertexArrays(1, &quadVAO);
    glDeleteProgram(Program);
}

void demo_instancing::Update(const platform_io& IO)
{
    const float AspectRatio = (float)IO.WindowWidth / (float)IO.WindowHeight;
    glViewport(0, 0, IO.WindowWidth, IO.WindowHeight);

    Camera = CameraUpdateFreefly(Camera, IO.CameraInputs);

    // Clear screen
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.0f, 0.0f, 0.0f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    mat4 ProjectionMatrix = Mat4::Perspective(Math::ToRadians(60.f), AspectRatio, 0.1f, 100.f);
    mat4 ViewMatrix = CameraGetInverseMatrix(Camera);
    mat4 ModelMatrix = Mat4::Translate({ 0.f, 0.f, 0.f });

    // Render tavern
    this->RenderScene(ProjectionMatrix, ViewMatrix, ModelMatrix);

    // Display debug UI
    this->DisplayDebugUI();
}

void demo_instancing::DisplayDebugUI()
{
    if (ImGui::TreeNodeEx("demo_base", ImGuiTreeNodeFlags_Framed))
    {
        // Debug display
        ImGui::Checkbox("Wireframe", &Wireframe);
        if (ImGui::TreeNodeEx("Camera"))
        {
            ImGui::Text("Position: (%.2f, %.2f, %.2f)", Camera.Position.x, Camera.Position.y, Camera.Position.z);
            ImGui::Text("Pitch: %.2f", Math::ToDegrees(Camera.Pitch));
            ImGui::Text("Yaw: %.2f", Math::ToDegrees(Camera.Yaw));
            ImGui::TreePop();
        }
        ImGui::DragInt("Instance count", &InstanceCount);

        InspectLights();

        ImGui::TreePop();
    }
}

void demo_instancing::GenMatrices()
{
    static int previousCount = 0;
    static std::vector<v2> speeds;
    static std::vector<v2> transforms;
    static std::vector<v3> displacements;

    std::vector<mat4> modelMatrices;
    modelMatrices.resize(InstanceCount);
    
    bool newRange = previousCount != InstanceCount;
    previousCount = InstanceCount;

    if (newRange)
    {
        speeds.resize(InstanceCount);
        displacements.resize(InstanceCount);
        transforms.resize(InstanceCount);
    }
    // Instance matrices
    {

        srand(ImGui::GetTime());
        float radius = 50.f;
        float offset = 25.f;

        auto displace = [](float offset)->float { return (rand() % (int)(offset * 100.f)) / 100.f - offset;  };

        for (int i = 0; i < InstanceCount; i++)
        {
            if (newRange)
            {
                speeds[i] = { 0.f, (float)(rand() % 100) / 10000.f };
                displacements[i] = { displace(offset) , displace(offset) , displace(offset) };
                transforms[i] = { (rand() % 20) / 100.0f + 0.05f , (float)(rand() % 360) };
            }

            speeds[i].x += speeds[i].y;

            mat4 model = Mat4::Identity();
            // position
            float angle = (float)i / (float)INSTANCE * 360.f + speeds[i].x;
            float x = sin(angle) * radius + displacements[i].x;
            float y = displacements[i].y * 0.4f;
            float z = cos(angle) * radius + displacements[i].z;
            model = Mat4::Translate(v3{ x, y, z });

            // scale
            float scale = transforms[i].x;
            model = model * Mat4::Scale(v3{ scale, scale, scale });

            //rotation
            float rotation = transforms[i].y;
            model = model * Mat4::RotateX(rotation);

            modelMatrices[i] = model;
        }
    }

    // Generate VBO
    {
        glBindVertexArray(asteroid.VAO);

        GLuint instanceVBO = 0;
        glGenBuffers(1, &instanceVBO);
        glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);

        glBufferData(GL_ARRAY_BUFFER, InstanceCount * sizeof(mat4), &modelMatrices.data()[0], GL_STATIC_DRAW);

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
}

void demo_instancing::RenderScene(const mat4& ProjectionMatrix, const mat4& ViewMatrix, const mat4& ModelMatrix)
{
    // Use shader and configure its uniforms
    glUseProgram(Program);

    // Set uniforms
    mat4 NormalMatrix = Mat4::Transpose(Mat4::Inverse(ModelMatrix));
    glUniformMatrix4fv(glGetUniformLocation(Program, "uProjection"), 1, GL_FALSE, ProjectionMatrix.e);
    glUniformMatrix4fv(glGetUniformLocation(Program, "uModel"), 1, GL_FALSE, ModelMatrix.e);
    glUniformMatrix4fv(glGetUniformLocation(Program, "uView"), 1, GL_FALSE, ViewMatrix.e);
    glUniformMatrix4fv(glGetUniformLocation(Program, "uModelNormalMatrix"), 1, GL_FALSE, NormalMatrix.e);
    glUniform3fv(glGetUniformLocation(Program, "uViewPosition"), 1, Camera.Position.e);

    // Bind uniform buffer and textures
    glBindBufferBase(GL_UNIFORM_BUFFER, LIGHT_BLOCK_BINDING_POINT, LightsUniformBuffer);

    GenMatrices();

    RenderAsteroids();
}

void demo_instancing::RenderQuad()
{
    glBindVertexArray(quadVAO);
    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, INSTANCE);
    glBindVertexArray(0);
}

void demo_instancing::RenderAsteroids()
{
    glUseProgram(Program);
    asteroid.Draw(InstanceCount);
}


static bool EditLight(GL::light* Light)
{
    bool Result =
        ImGui::Checkbox("Enabled", (bool*)&Light->Enabled)
        + ImGui::SliderFloat4("Position", Light->Position.e, -4.f, 4.f)
        + ImGui::ColorEdit3("Ambient", Light->Ambient.e)
        + ImGui::ColorEdit3("Diffuse", Light->Diffuse.e)
        + ImGui::ColorEdit3("Specular", Light->Specular.e)
        + ImGui::SliderFloat("Attenuation (constant)", &Light->Attenuation.e[0], 0.f, 10.f)
        + ImGui::SliderFloat("Attenuation (linear)", &Light->Attenuation.e[1], 0.f, 10.f)
        + ImGui::SliderFloat("Attenuation (quadratic)", &Light->Attenuation.e[2], 0.f, 10.f);
    return Result;
}

void demo_instancing::InspectLights()
{
    if (ImGui::TreeNodeEx("Lights"))
    {
        for (int i = 0; i < LightCount; ++i)
        {
            if (ImGui::TreeNode(&Lights[i], "Light[%d]", i))
            {
                GL::light& Light = Lights[i];
                if (EditLight(&Light))
                {
                    glBufferSubData(GL_UNIFORM_BUFFER, i * sizeof(GL::light), sizeof(GL::light), &Light);
                }

                // Calculate attenuation based on the light values
                if (ImGui::TreeNode("Attenuation calculator"))
                {
                    static float Dist = 5.f;
                    float Att = 1.f / (Light.Attenuation.e[0] + Light.Attenuation.e[1] * Dist + Light.Attenuation.e[2] * Light.Attenuation.e[2] * Dist);
                    ImGui::Text("att(d) = 1.0 / (c + ld + qdd)");
                    ImGui::SliderFloat("d", &Dist, 0.f, 20.f);
                    ImGui::Text("att(%.2f) = %.2f", Dist, Att);
                    ImGui::TreePop();
                }
                ImGui::TreePop();
            }
        }
        ImGui::TreePop();
    }
}
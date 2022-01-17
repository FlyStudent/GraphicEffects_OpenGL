
#include <vector>

#include <imgui.h>

#include "opengl_helpers.h"
#include "opengl_helpers_wireframe.h"

#include "color.h"
#include "maths.h"
#include "mesh.h"

#include "demo_normalmapping.h"

const int LIGHT_BLOCK_BINDING_POINT = 0;

#pragma region BASIC VS
static const char* gVertexShaderStr = R"GLSL(
// Attributes
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aUV;
layout(location = 2) in vec3 aNormal;
layout(location = 3) in vec3 aTangent;

// Uniforms
uniform mat4 uProjection;
uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uModelNormalMatrix;

// Varyings
out vec2 vUV;
out vec3 vPos;    // Vertex position in view-space
out vec3 vNormal; // Vertex normal in view-space
out mat3 TBN;

void main()
{
    vUV = aUV;
    vec4 pos4 = (uModel * vec4(aPosition, 1.0));
    vPos = pos4.xyz / pos4.w;
    vNormal = (uModelNormalMatrix * vec4(aNormal, 0.0)).xyz;
    gl_Position = uProjection * uView * pos4;

    vec3 T = normalize(vec3(uModel * vec4(aTangent,   0.0)));
    vec3 N = normalize(vec3(uModel * vec4(aNormal,    0.0)));
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T);
    TBN = mat3(T, B, N);    

})GLSL";
#pragma endregion

#pragma region BASIC FS
static const char* gFragmentShaderStr = R"GLSL(

// Varyings
in vec2 vUV;
in vec3 vPos;
in vec3 vNormal;
in mat3 TBN;

// Uniforms
uniform mat4 uProjection;
uniform vec3 uViewPosition;
uniform bool uProcessNormalMap;


uniform sampler2D uDiffuseTexture;
uniform sampler2D uNormalTexture;

// Uniform blocks
layout(std140) uniform uLightBlock
{
	light uLight[LIGHT_COUNT];
};

// Shader outputs
out vec4 oColor;

vec3 normal;

light_shade_result get_lights_shading()
{
    light_shade_result lightResult = light_shade_result(vec3(0.0), vec3(0.0), vec3(0.0));
	for (int i = 0; i < LIGHT_COUNT; ++i)
    {
        light_shade_result light = light_shade(uLight[i], gDefaultMaterial.shininess, uViewPosition, vPos, normalize(normal));
        lightResult.ambient  += light.ambient;
        lightResult.diffuse  += light.diffuse;
        lightResult.specular += light.specular;
    }
    return lightResult;
}

void main()
{
    normal = vNormal;
    if (uProcessNormalMap)
    {
        normal = texture(uNormalTexture, vUV).rgb;
        normal = normalize(normalize(normal) * 2.0 - 1.0);
        normal = normalize(TBN * normal);
    }

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

bag_object::bag_object(GL::cache& GLCache)
{
    // Create mesh
    {
        // Use vbo from GLCache
        MeshBuffer = GLCache.LoadObj("media/bag/bag.obj", 1.f, &this->MeshVertexCount, &MeshDesc);
    }

    // Gen texture
    {
        DiffuseTexture = GLCache.LoadTexture("media/bag/bag_diffuse.jpg", IMG_FLIP | IMG_GEN_MIPMAPS);
        //SDiffuseTexture = GLCache.LoadTexture("media/fantasy_game_inn_diffuse.png", IMG_FLIP | IMG_GEN_MIPMAPS, (int*)nullptr, (int*)nullptr, true);
        NormalTexture = GLCache.LoadTexture("media/bag/bag_normal.png", IMG_FLIP | IMG_GEN_MIPMAPS);
    }
}


demo_normalmapping::demo_normalmapping(GL::cache& GLCache, GL::debug& GLDebug)
    : GLDebug(GLDebug), BagObject(GLCache)
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

    // Create bag object
    {
        glGenVertexArrays(1, &BagObject.MeshArrayObject);
        glBindVertexArray(BagObject.MeshArrayObject);

        glBindBuffer(GL_ARRAY_BUFFER, BagObject.MeshBuffer);

        vertex_descriptor& Desc = BagObject.MeshDesc;
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, Desc.Stride, (void*)(size_t)Desc.PositionOffset);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, Desc.Stride, (void*)(size_t)Desc.UVOffset);
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, Desc.Stride, (void*)(size_t)Desc.NormalOffset);
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, Desc.Stride, (void*)(size_t)Desc.TangentOffset);
    }

    // (Default light, standard values)
    {
        GL::light DefaultLight = {};
        DefaultLight.Enabled = true;
        DefaultLight.Position = { -0.0f, 0.0f, -1.5f, 1.f };
        DefaultLight.Ambient = { 0.2f, 0.2f, 0.2f };
        DefaultLight.Diffuse = { 1.0f, 1.0f, 1.0f };
        DefaultLight.Specular = { 0.0f, 0.0f, 0.0f };
        DefaultLight.Attenuation = { 1.0f, 0.0f, 0.0f };

        // Sun light
        this->Lights[0] = DefaultLight;
    }

    Texture = GLCache.LoadTexture("media/brick.png", IMG_FLIP | IMG_GEN_MIPMAPS);
    NormalTexture = GLCache.LoadTexture("media/bricknormal.png", IMG_FLIP | IMG_GEN_MIPMAPS);

    // Create a quad Vertex Object
    {
        // positions
        v3 pos1 = { -1.0, 1.0, 0.0 };
        v3 pos2 = { -1.0, -1.0, 0.0 };
        v3 pos3 = { 1.0, -1.0, 0.0 };
        v3 pos4 = { 1.0, 1.0, 0.0 };
        // texture coordinates
        v2 uv1 = {0.0, 1.0};
        v2 uv2 = {0.0, 0.0};
        v2 uv3 = {1.0, 0.0};
        v2 uv4 = {1.0, 1.0};
        // normal vector
        v3 nm = { 0.0, 0.0, 1.0 };

        v3 tangent1, tangent2;

        // TRIANGLE 1
        v3 edge1 = pos2 - pos1;
        v3 edge2 = pos3 - pos1;
        v2 deltaUV1 = uv2 - uv1;
        v2 deltaUV2 = uv3 - uv1;

        float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

        tangent1.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
        tangent1.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
        tangent1.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);

        // TRIANGLE 2
        edge1 = pos3 - pos1;
        edge2 = pos4 - pos1;
        deltaUV1 = uv3 - uv1;
        deltaUV2 = uv4 - uv1;

        f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

        tangent2.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
        tangent2.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
        tangent2.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);

        float quadVertices[] = {
            // positions            // texcoords  // normal         // tangent                          // bitangent
            pos1.x, pos1.y, pos1.z, uv1.x, uv1.y, nm.x, nm.y, nm.z, tangent1.x, tangent1.y, tangent1.z,
            pos2.x, pos2.y, pos2.z, uv2.x, uv2.y, nm.x, nm.y, nm.z, tangent1.x, tangent1.y, tangent1.z,
            pos3.x, pos3.y, pos3.z, uv3.x, uv3.y, nm.x, nm.y, nm.z, tangent1.x, tangent1.y, tangent1.z,
                                   
            pos1.x, pos1.y, pos1.z, uv1.x, uv1.y,  nm.x, nm.y, nm.z, tangent2.x, tangent2.y, tangent2.z,
            pos3.x, pos3.y, pos3.z, uv3.x, uv3.y, nm.x, nm.y, nm.z, tangent2.x, tangent2.y, tangent2.z,
            pos4.x, pos4.y, pos4.z, uv4.x, uv4.y, nm.x, nm.y, nm.z, tangent2.x, tangent2.y, tangent2.z,
        };                          

        // setup plane VAO
        GLuint quadVBO = 0;
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);

        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);

        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(5 * sizeof(float)));
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(8 * sizeof(float)));
    }

    // Set uniforms that won't change
    {
        glUseProgram(Program);
        glUniform1i(glGetUniformLocation(Program, "uDiffuseTexture"), 0);
        glUniform1i(glGetUniformLocation(Program, "uNormalTexture"), 1);
        glUniformBlockBinding(Program, glGetUniformBlockIndex(Program, "uLightBlock"), LIGHT_BLOCK_BINDING_POINT);
    }

    // Gen light uniform buffer
    {
        glGenBuffers(1, &LightsUniformBuffer);
        glBindBuffer(GL_UNIFORM_BUFFER, LightsUniformBuffer);
        glBufferData(GL_UNIFORM_BUFFER, LightCount * sizeof(GL::light), Lights.data(), GL_DYNAMIC_DRAW);
    }
}

demo_normalmapping::~demo_normalmapping()
{
    // Cleanup GL
    glDeleteVertexArrays(1, &quadVAO);
    glDeleteProgram(Program);
}

void demo_normalmapping::Update(const platform_io& IO)
{
    const float AspectRatio = (float)IO.WindowWidth / (float)IO.WindowHeight;
    glViewport(0, 0, IO.WindowWidth, IO.WindowHeight);

    Camera = CameraUpdateFreefly(Camera, IO.CameraInputs);

    // Clear screen
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.f, 0.f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    mat4 ProjectionMatrix = Mat4::Perspective(Math::ToRadians(60.f), AspectRatio, 0.1f, 100.f);
    mat4 ViewMatrix = CameraGetInverseMatrix(Camera);

    // TRS
    mat4 ModelMatrix = Mat4::Translate(Position);
    ModelMatrix = ModelMatrix * Mat4::RotateX(Math::ToRadians(Rotation.x)) * Mat4::RotateY(Math::ToRadians(Rotation.y)) * Mat4::RotateZ(Math::ToRadians(Rotation.z));
    ModelMatrix = ModelMatrix * Mat4::Scale(v3{ Scale, Scale, Scale });

    // Render tavern
    this->RenderScene(ProjectionMatrix, ViewMatrix, ModelMatrix);

    // Display debug UI
    this->DisplayDebugUI();
}

void demo_normalmapping::DisplayDebugUI()
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
        if (ImGui::TreeNodeEx("Objet"))
        {
            ImGui::DragFloat3("Position: ", &Position.x);
            ImGui::DragFloat("Scale: ", &Scale, 0.01f, 0.01f, 100.f);
            ImGui::DragFloat3("Rotation: ", &Rotation.x);
            ImGui::TreePop();
        }

        ImGui::Checkbox("Normal mapping", &NormalMapping);

        InspectLights();

        ImGui::TreePop();
    }
}

void demo_normalmapping::RenderScene(const mat4& ProjectionMatrix, const mat4& ViewMatrix, const mat4& ModelMatrix)
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

    glUniform1i(glGetUniformLocation(Program, "uProcessNormalMap"), (GLint)NormalMapping);

    glBindBufferBase(GL_UNIFORM_BUFFER, LIGHT_BLOCK_BINDING_POINT, LightsUniformBuffer);

    // Draw mesh
    //RenderQuad();
    RenderBag();

}

void demo_normalmapping::RenderQuad()
{
    // Bind uniform buffer and textures
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, Texture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, NormalTexture);
    glActiveTexture(GL_TEXTURE0);

    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

void demo_normalmapping::RenderBag()
{
    glEnable(GL_DEPTH_TEST);

    // Use shader and configure its uniforms
    glUseProgram(Program);

    // Bind uniform buffer and textures
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, BagObject.DiffuseTexture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, BagObject.NormalTexture);
    glActiveTexture(GL_TEXTURE0); // Reset active texture just in case

    // Draw mesh
    glBindVertexArray(BagObject.MeshArrayObject);
    glDrawArrays(GL_TRIANGLES, 0, BagObject.MeshVertexCount);
    glBindVertexArray(0);
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

void demo_normalmapping::InspectLights()
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
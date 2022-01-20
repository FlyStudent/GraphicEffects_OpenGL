
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

#pragma region SHADERS
const int LIGHT_BLOCK_BINDING_POINT = 0;
static const char* gVertexShaderStr = R"GLSL(
// Attributes
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aUV;
layout(location = 2) in vec3 aNormal;

// Uniforms
uniform highp mat4 uProjection;
uniform highp mat4 uModel;
uniform highp mat4 uView;
uniform highp mat4 uModelNormalMatrix;

// Varyings
out vec2 vUV;
out vec3 vPos;    // Vertex position in view-space
out vec3 vNormal; // Vertex normal in view-space
out int id;

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


static const char* gVertexShaderMPStr = R"GLSL(
// Attributes
layout(location = 0) in vec3 aPosition;

// Uniforms
uniform mat4 uProjection;
uniform mat4 uModel;
uniform mat4 uView;

void main()
{

    vec4 pos4 = (uModel * vec4(aPosition, 1.0));
    gl_Position = uProjection * uView * pos4;
})GLSL";

static const char* gFragmentShaderMPStr = R"GLSL(

// Varyings
out vec4 color;
uniform vec4 Inid;


void main()
{
    color = Inid;
})GLSL";

#pragma endregion

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

        char FragmentShaderConfigs[] = "#define LIGHT_COUNT %d\n";
        snprintf(FragmentShaderConfigs, ARRAY_SIZE(FragmentShaderConfigs), "#define LIGHT_COUNT %d\n", TavernScene.LightCount);
        const char* FragmentShaderStrss[2] = {
            FragmentShaderConfigs,
            gFragmentShaderMPStr,
        };

        this->SkyProgram = GL::CreateProgram(gVertexShaderCubeStr, gFragmentShaderCubeStr);
        this->ReflectiveProgram = GL::CreateProgramEx(1, &gVertexShaderStr, 2, FragmentShaderStrs, true);
        this->MousePickingProgram = GL::CreateProgramEx(1, &gVertexShaderMPStr, 2, FragmentShaderStrss, true);
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
        vertex_descriptor sphere;
        GLuint buffer = GLCache.LoadObj("media/sphere.obj",1.f, &VertexMeshCount, &sphere);

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
        /*"media/Sky_NightTime01FT.png",
        "media/Sky_NightTime01BK.png",
        "media/Sky_NightTime01UP.png",
        "media/Sky_NightTime01DN.png",
        "media/Sky_NightTime01RT.png",
        "media/Sky_NightTime01LF.png",*/
    "media/right.jpg",
    "media/left.jpg",
    "media/top.jpg",
    "media/bottom.jpg",
    "media/front.jpg",
    "media/back.jpg"
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
            }
                stbi_image_free(data);

            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        }
        glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    }
    
    GenerateCubemap(EnvironmentTexture, 128.f, 128.f, GL_RGB, GL_UNSIGNED_BYTE);
    GenerateCubemap(DepthTexture, 1024.f, 1024.f, GL_DEPTH_COMPONENT, GL_FLOAT);

    vertex_descriptor Descriptor = {};
    Descriptor.Stride = sizeof(vertex_full);
    Descriptor.HasUV = true;
    Descriptor.PositionOffset = OFFSETOF(vertex_full, Position);
    Descriptor.UVOffset = OFFSETOF(vertex_full, UV);
    Descriptor.NormalOffset = OFFSETOF(vertex_full, Normal);
    // Create cube vertices
    vertex_full Cube[36];

    Mesh::BuildCube(Cube, Cube + 36, Descriptor);

    // Upload cube to gpu (VRAM)

    GLuint cube = 0;
    glGenBuffers(1, &cube);
    glBindBuffer(GL_ARRAY_BUFFER, cube);
    glBufferData(GL_ARRAY_BUFFER, 36 * sizeof(vertex_full), Cube, GL_STATIC_DRAW);

    // Create a vertex array
    glGenVertexArrays(1, &CubeVAO);
    glBindVertexArray(CubeVAO);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_full), (void*)(Descriptor.PositionOffset));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_full), (void*)(Descriptor.NormalOffset));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_full), (void*)(Descriptor.UVOffset));

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    
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

void demo_skybox::RenderSkybox(const camera& cam, const mat4& projection) 
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
    glUseProgram(0);
}

void demo_skybox::MousePicking(const platform_io& IO)
{
    // Compute Matrixs
    mat4 ViewMatrix = CameraGetInverseMatrix(Camera);
    mat4 ModelMatrix = Mat4::Translate({ 0.f,0.f,0.f });
    mat4 NormalMatrix = Mat4::Transpose(Mat4::Inverse(ModelMatrix));

    int i = 0;

    // Clear buffers
    glClearColor(0.f, 0.f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    mat4 ProjectionMatrix = Mat4::Perspective(Math::ToRadians(60.f), 1.f, 0.1f, 100.f);

    RenderTavernEx(ProjectionMatrix, ViewMatrix, ModelMatrix, i);

    mat4 model = Mat4::Translate(Position);
    i += 1;
    glUseProgram(MousePickingProgram);

    glUniformMatrix4fv(glGetUniformLocation(MousePickingProgram, "uProjection"), 1, GL_FALSE, ProjectionMatrix.e);
    glUniformMatrix4fv(glGetUniformLocation(MousePickingProgram, "uModel"), 1, GL_FALSE, model.e);
    glUniformMatrix4fv(glGetUniformLocation(MousePickingProgram, "uView"), 1, GL_FALSE, ViewMatrix.e);
    v3 color = Color::RGB(i);
    glUniform4f(glGetUniformLocation(MousePickingProgram, "Inid"), color.r, color.g, color.b, 1.f);

    // Draw mesh
    glBindVertexArray(SphereVAO);
    glDrawArrays(GL_TRIANGLES, 0, 2880);

    glFlush();
    glFinish();

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    unsigned char data[3];
    glReadPixels(IO.MouseX, IO.DeltaMouseY, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, data);
    int pickedID =
        data[0] +
        data[1] * 256 +
        data[2] * 256 * 256;

    std::cout << pickedID << std::endl;
}

void demo_skybox::GenerateCubemap(GLuint& index, const float width,const float height, const GLint format, const GLint size)
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

void demo_skybox::RenderSceneWithReflection(const mat4& ProjectionMatrix, const mat4& ModelMatrix, const camera& cam)
{

    RenderScene(ProjectionMatrix, ModelMatrix, cam.Position, cam);
    mat4 ViewMatrix = CameraGetInverseMatrix(cam);
    mat4 NormalMatrix = Mat4::Transpose(Mat4::Inverse(ModelMatrix));

    // Render Sphere
    glUseProgram(ReflectiveProgram);
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

}

void demo_skybox::RenderScene(const mat4& ProjectionMatrix, const mat4& ModelMatrix,const v3& position, const camera& cam)
{
    // Compute Matrixs
    mat4 ViewMatrix = CameraGetInverseMatrix(cam);
    mat4 NormalMatrix = Mat4::Transpose(Mat4::Inverse(ModelMatrix));

    // Clear buffers
    glClearColor(0.f, 0.f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    RenderSkybox(cam, ProjectionMatrix);
    RenderTavern(ProjectionMatrix, ViewMatrix, ModelMatrix);

    mat4 model = Mat4::Translate(Position);

    glUseProgram(Program);

    glUniformMatrix4fv(glGetUniformLocation(Program, "uProjection"), 1, GL_FALSE, ProjectionMatrix.e);
    glUniformMatrix4fv(glGetUniformLocation(Program, "uModel"), 1, GL_FALSE, model.e);
    glUniformMatrix4fv(glGetUniformLocation(Program, "uView"), 1, GL_FALSE, ViewMatrix.e);
    glUniformMatrix4fv(glGetUniformLocation(Program, "uModelNormalMatrix"), 1, GL_FALSE, NormalMatrix.e);
    glUniform3fv(glGetUniformLocation(Program, "uViewPosition"), 1, cam.Position.e);

    // Bind uniform buffer and textures
    glBindBufferBase(GL_UNIFORM_BUFFER, LIGHT_BLOCK_BINDING_POINT, TavernScene.LightsUniformBuffer);

    // Draw mesh
    glBindVertexArray(SphereVAO);
    glDrawArrays(GL_TRIANGLES, 0, 2880);

    model = CameraGetMatrixEx(Camera, {0.f, -0.75f, 0.f});
    glUniformMatrix4fv(glGetUniformLocation(Program, "uModel"), 1, GL_FALSE, model.e);
    glBindVertexArray(CubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
}

void demo_skybox::RenderEnvironmentMap(const v3& center) 
    //Update environment skybox 
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
        mat4 ProjectionMatrix = Mat4::Perspective(Math::ToRadians(-90.f), 1.f, 0.1f, 100.f);
        mat4 ViewMatrix = CameraGetInverseMatrix(RenderingCamera);
        mat4 ModelMatrix = Mat4::Translate({ 0.f, 0.f, 0.f });
        mat4 ViewMatrixWT = CameraGetInverseMatrixWT(RenderingCamera);

        RenderScene(ProjectionMatrix, ModelMatrix,RenderingCamera.Position, RenderingCamera);
    }
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void demo_skybox::RenderDepthMap() 
{
    GLuint depthMapFBO;

    glGenFramebuffers(1, &depthMapFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, DepthTexture, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);


    glViewport(0, 0, 1024.f, 1024.f);
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);

    mat4 shadowProj = Mat4::Ortho(-10.0f, 10.0f, -10.0f, 10.0f, 1.0f, 7.5f);
    RenderScene(shadowProj, Mat4::Identity(), TavernScene.Lights[0].Position.rgb);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void demo_skybox::Update(const platform_io& IO)
{
    if (IO.hasJustClicked) 
    {
        MousePicking(IO);
    }

    // Render New Cubemap For reflection
    RenderEnvironmentMap({ 0,0,0 });

    //Compute AspectRatio
    const float AspectRatio = (float)IO.WindowWidth / (float)IO.WindowHeight;
    glViewport(0, 0, IO.WindowWidth, IO.WindowHeight);


    // Update Camera and misc
    Camera = CameraUpdateFreefly(Camera, IO.CameraInputs);
    Position = { -2.f, Math::Cos(IO.Time) * -5.f, 0.f };

    //Compute basic matrix
    mat4 ProjectionMatrix = Mat4::Perspective(Math::ToRadians(60.f), AspectRatio, 0.1f, 100.f);
    mat4 ModelMatrix = Mat4::Translate({ 0.f, 0.f, 0.f });

    RenderSceneWithReflection(ProjectionMatrix, ModelMatrix, Camera);
    
    // Display debug UI
    this->DisplayDebugUI();
}

void demo_skybox::DisplayDebugUI()
{
    if (ImGui::TreeNodeEx("demo_Skybox", ImGuiTreeNodeFlags_Framed))
    {
        // Debug display
        ImGui::Checkbox("Dynamic Reflection", &Dynamic);
        TavernScene.InspectLights();

        ImGui::TreePop();
    }
}


void demo_skybox::RenderTavernEx(const mat4& ProjectionMatrix, const mat4& ViewMatrix, const mat4& ModelMatrix, int i)
{
    glEnable(GL_DEPTH_TEST);

    // Use shader and configure its uniforms
    glUseProgram(MousePickingProgram);

    // Set uniforms
    glUniformMatrix4fv(glGetUniformLocation(MousePickingProgram, "uProjection"), 1, GL_FALSE, ProjectionMatrix.e);
    glUniformMatrix4fv(glGetUniformLocation(MousePickingProgram, "uModel"), 1, GL_FALSE, ModelMatrix.e);
    glUniformMatrix4fv(glGetUniformLocation(MousePickingProgram, "uView"), 1, GL_FALSE, ViewMatrix.e);
    v3 color = Color::RGB(i);
    glUniform4f(glGetUniformLocation(MousePickingProgram, "Inid"), color.r, color.g, color.b, 1.f);

    // Draw mesh
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, TavernScene.MeshVertexCount);
}
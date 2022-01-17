
#include "platform.h"

#include "opengl_helpers.h"

#include "opengl_helpers_cache.h"

GL::cache::cache()
{
	MeshDesc.Stride = sizeof(vertex_full);
	MeshDesc.HasNormal = true;
	MeshDesc.HasUV = true;
	MeshDesc.HasTangent = true;
	MeshDesc.PositionOffset = OFFSETOF(vertex_full, Position);
	MeshDesc.UVOffset = OFFSETOF(vertex_full, UV);
	MeshDesc.NormalOffset = OFFSETOF(vertex_full, Normal);
	MeshDesc.TangentOffset = OFFSETOF(vertex_full, Tangent);
}

GL::cache::~cache()
{
	for (const auto& KeyValue : this->TextureMap)
		glDeleteTextures(1, &KeyValue.second.TextureID);

	for (const auto& KeyValue : this->VertexBufferMap)
		glDeleteBuffers(1, &KeyValue.second.VertexBuffer);
}

GLuint GL::cache::LoadObj(const char* Filename, float Scale, int* VertexCountOut, vertex_descriptor* DescOut)
{
	auto Found = this->VertexBufferMap.find(Filename);
	if (Found != this->VertexBufferMap.end())
	{
		if (VertexCountOut)
			*VertexCountOut = Found->second.Size;
		if (DescOut)
			*DescOut = MeshDesc;
		return Found->second.VertexBuffer;
	}

	this->TmpBuffer.clear();
	Mesh::LoadObjNoConvertion(this->TmpBuffer, Filename, Scale);

	// Upload mesh to gpu
	GLuint MeshBuffer = 0;
	glGenBuffers(1, &MeshBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, MeshBuffer);
	glBufferData(GL_ARRAY_BUFFER, this->TmpBuffer.size() * sizeof(vertex_full), &this->TmpBuffer[0], GL_STATIC_DRAW);

	if (VertexCountOut)
		*VertexCountOut = (int)this->TmpBuffer.size();

	if (DescOut)
		*DescOut = MeshDesc;
	
	this->VertexBufferMap[Filename] = { MeshBuffer, (int)this->TmpBuffer.size() };

	return MeshBuffer;
}

GLuint GL::cache::LoadTexture(const char* Filename, int ImageFlags, int* WidthOut, int* HeightOut, bool gamma)
{
	texture_identifier TextureIdentifier = { Filename, ImageFlags };
	
	auto Found = this->TextureMap.find(TextureIdentifier);
	if (Found != this->TextureMap.end())
	{
		if (WidthOut)  *WidthOut  = Found->second.Width;
		if (HeightOut) *HeightOut = Found->second.Height;
		return Found->second.TextureID;
	}

	GLuint Texture;
	glGenTextures(1, &Texture);
	glBindTexture(GL_TEXTURE_2D, Texture);
	int Width, Height;
	if (gamma)
		GL::UploadGammaTexture(Filename, ImageFlags, &Width, &Height);
	else
		GL::UploadTexture(Filename, ImageFlags, &Width, &Height);

	if (WidthOut)  *WidthOut  = Width;
	if (HeightOut) *HeightOut = Height;

	this->TextureMap[TextureIdentifier] = { Texture, Width, Height };

	return Texture;
}

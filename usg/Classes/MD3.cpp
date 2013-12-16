#include "Calc.h"
#include "Debugging.h"
#include "Exception.h"
#include "MD3.h"
#include "SpriteRenderer.h"
#include "Stream.h"
#include "StreamReader.h"

class MD3Header
{
public:
	char Magic[4];
	int Version;
	char Name[64];
	int Flags;
	int FrameCount;
	int TagCount;
	int SurfaceCount;
	int SkinCount;
	int FrameOffset;
	int TagOffset;
	int SurfaceOffset;
	int End;

	void Read(StreamReader& reader)
	{
		reader.ReadBytes(Magic, 4);
		Version = reader.ReadInt32();
		reader.ReadBytes(Name, 64);
		Flags = reader.ReadInt32();
		FrameCount = reader.ReadInt32();
		TagCount = reader.ReadInt32();
		SurfaceCount = reader.ReadInt32();
		SkinCount = reader.ReadInt32();
		FrameOffset = reader.ReadInt32();
		TagOffset = reader.ReadInt32();
		SurfaceOffset = reader.ReadInt32();
		End = reader.ReadInt32();
		
		Assert(memcmp(Magic, "IDP3", 4) == 0);
	}
};

class MD3Surface
{
public:
	long Offset;

	char Magic[4];
	char Name[64];
	int Flags;
	int FrameCount;
	int ShaderCount;
	int VertexCount;
	int TriangleCount;
	int TriangleOffset;
	int ShaderOffset;
	int TexcoordOffset;
	int VertexOffset;
	int End;

	void Read(StreamReader& reader)
	{
		reader.ReadBytes(Magic, 4);
		reader.ReadBytes(Name, 64);
		Flags = reader.ReadInt32();
		FrameCount = reader.ReadInt32();
		ShaderCount = reader.ReadInt32();
		VertexCount = reader.ReadInt32();
		TriangleCount = reader.ReadInt32();
		TriangleOffset = reader.ReadInt32();
		ShaderOffset = reader.ReadInt32();
		TexcoordOffset = reader.ReadInt32();
		VertexOffset = reader.ReadInt32();
		End = reader.ReadInt32();
		
		Assert(memcmp(Magic, "IDP3", 4) == 0);
	}
};

MD3Model* MD3Loader::Load(Stream* stream)
{
	StreamReader reader(stream, false);
	
	MD3Model* model = new MD3Model();
	
	MD3Header h;
	h.Read(reader);

	MD3Surface* surfaceList = new MD3Surface[h.SurfaceCount];
	model->SurfList = new MD3Surf[h.SurfaceCount];
	model->SurfCount = h.SurfaceCount;
	stream->Seek(h.SurfaceOffset, SeekMode_Begin);
	for (unsigned int i = 0; i < model->SurfCount; ++i)
	{
		MD3Surface& s = surfaceList[i];
		MD3Surf& surf = model->SurfList[i];

		s.Offset = stream->Position_get();
		s.Read(reader);

		// read vertex list

		stream->Seek(int(s.Offset + s.VertexOffset), SeekMode_Begin);

		Assert(s.VertexCount <= 65535);
		
		surf.VertCount = s.VertexCount;
		surf.VertList = new MD3Vert[s.VertexCount];
		
		for (uint32_t j = 0; j < surf.VertCount; ++j)
		{
			short p[3];
			uint8_t n[2];

			p[0] = reader.ReadInt16();
			p[1] = reader.ReadInt16();
			p[2] = reader.ReadInt16();
			n[0] = reader.ReadUInt8();
			n[1] = reader.ReadUInt8();

			MD3Vert& v = surf.VertList[j];
			
			v.p[0] = p[0] / 64.0f;
			v.p[1] = p[1] / 64.0f;
			v.p[2] = p[2] / 64.0f;
			
			float a1 = n[0] / 255.0f * Calc::m2PI;
			float a2 = n[1] / 255.0f * Calc::m2PI;
			v.n[0] = cosf(a2) * sinf(a1);
			v.n[1] = sinf(a2) * sinf(a1);
			v.n[2] = cosf(a1);
		}

		// read UV coord list

		stream->Seek(int(s.Offset + s.TexcoordOffset), SeekMode_Begin);

		for (uint32_t j = 0; j < surf.VertCount; ++j)
		{
			surf.VertList[j].uv[0] = reader.ReadFloat();
			surf.VertList[j].uv[1] = reader.ReadFloat();
		}

		// read index list

		stream->Seek(int(s.Offset + s.TriangleOffset), SeekMode_Begin);

		surf.IndexCount = s.TriangleCount * 3;
		surf.IndexList = new uint16_t[s.TriangleCount * 3];

		for (uint32_t j = 0; j < surf.IndexCount; ++j)
		{
			int32_t index = reader.ReadInt32();

			Assert(index >= 0 && index < s.VertexCount && index <= 65535);

			surf.IndexList[j] = uint16_t(index);
		}

		// end

		stream->Seek(int(s.Offset + s.End), SeekMode_Begin);
	}
	
	delete[] surfaceList;
	surfaceList = 0;

	return model;
}

void RenderMD3(MD3Model* model)
{
	for (uint32_t i = 0; i < model->SurfCount; ++i)
	{
		MD3Surf& surf = model->SurfList[i];
		
		SpriteRenderer::DrawTriangles_V3N3CT_Index(
			surf.VertList[0].p,
			sizeof(MD3Vert),
			surf.VertList[0].n,
			sizeof(MD3Vert),
			0,
			0,
			surf.VertList[0].uv,
			sizeof(MD3Vert),
			surf.IndexList,
			surf.IndexCount);
	}
}

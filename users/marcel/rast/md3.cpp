#include <assert.h>
#include <exception>
#include <stdio.h>
#include <string.h>
#include <xmmintrin.h>
#include "md3.h"

#define byte unsigned char

static FILE* gFile = 0;

static void ReadS8v(char* v, unsigned int n);
static char ReadS8();
static short ReadS16();
static int ReadS32();
static float ReadR32();

class Md3Header
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

	void Read()
	{
		ReadS8v(Magic, 4);
		Version = ReadS32();
		ReadS8v(Name, 64);
		Flags = ReadS32();
		FrameCount = ReadS32();
		TagCount = ReadS32();
		SurfaceCount = ReadS32();
		SkinCount = ReadS32();
		FrameOffset = ReadS32();
		TagOffset = ReadS32();
		SurfaceOffset = ReadS32();
		End = ReadS32();

		assert(!memcmp(Magic, "IDP3", 4));
	}
};

class Md3Surface
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

	void Read()
	{
		ReadS8v(Magic, 4);
		ReadS8v(Name, 64);
		Flags = ReadS32();
		FrameCount = ReadS32();
		ShaderCount = ReadS32();
		VertexCount = ReadS32();
		TriangleCount = ReadS32();
		TriangleOffset = ReadS32();
		ShaderOffset = ReadS32();
		TexcoordOffset = ReadS32();
		VertexOffset = ReadS32();
		End = ReadS32();

		assert(!memcmp(Magic, "IDP3", 4));
	}
};

void Md3::Load(const char* fileName)
{
#if defined(WINDOWS) && 0
	fopen_s(&gFile, fileName, "rb");
#else
	gFile = fopen(fileName, "rb");
#endif

	if (!gFile)
		throw std::exception();

	Md3Header h;
	h.Read();

	Md3Surface* surfaceList = new Md3Surface[h.SurfaceCount];
	SurfList = new DataSurf[h.SurfaceCount];
	SurfCount = h.SurfaceCount;
	fseek(gFile, h.SurfaceOffset, SEEK_SET);
	for (unsigned int i = 0; i < SurfCount; ++i)
	{
		Md3Surface& s = surfaceList[i];
		DataSurf& surf = SurfList[i];

		s.Offset = ftell(gFile);
		s.Read();

		// read vertex list

		fseek(gFile, s.Offset + s.VertexOffset, SEEK_SET);

		surf.VertCount = s.VertexCount;
		surf.VertList = (DataVert*)_mm_malloc(sizeof(DataVert) * s.VertexCount, 16);

		for (unsigned int j = 0; j < surf.VertCount; ++j)
		{
			short p[3];
			char n[2];

			p[0] = ReadS16();
			p[1] = ReadS16();
			p[2] = ReadS16();
			n[0] = ReadS8();
			n[1] = ReadS8();

			surf.VertList[j].p.Set4(
				p[0] / 64.0f,
				p[1] / 64.0f,
				p[2] / 64.0f,
				1.0f);
		}

		// read UV coord list

		fseek(gFile, s.Offset + s.TexcoordOffset, SEEK_SET);

		for (unsigned int j = 0; j < surf.VertCount; ++j)
		{
			surf.VertList[j].uv[0] = ReadR32();
			surf.VertList[j].uv[1] = ReadR32();
		}

		// read index list

		fseek(gFile, s.Offset + s.TriangleOffset, SEEK_SET);

		surf.IndexCount = s.TriangleCount * 3;
		surf.IndexList = new unsigned int[s.TriangleCount * 3];

		for (unsigned int j = 0; j < surf.IndexCount; ++j)
		{
			int index = ReadS32();

			assert(index >= 0 && index < s.VertexCount);

			surf.IndexList[j] = index;
		}

		// end

		fseek(gFile, s.Offset + s.End, SEEK_SET);
	}

	fclose(gFile);

	//

	SimdVec min(+1e10f);
	SimdVec max(-1e10f);

	for (unsigned int i = 0; i < SurfCount; ++i)
	{
		DataSurf& surf = SurfList[i];

		for (unsigned int j = 0; j < surf.VertCount; ++j)
		{
			min = min.Min(surf.VertList[j].p);
			max = max.Max(surf.VertList[j].p);
		}
	}

	printf("min (%g, %g, %g)\n", min.X(), min.Y(), min.Z());
	printf("max (%g, %g, %g)\n", max.X(), max.Y(), max.Z());
}

static void ReadS8v(char* v, unsigned int n)
{
	if (fread(v, 1, n, gFile) != (size_t)n)
		assert(false);
}

static char ReadS8()
{
	char r;
	if (fread(&r, 1, 1, gFile) != 1)
		assert(false);
	return r;
}

static short ReadS16()
{
	short r;
	if (fread(&r, 1, 2, gFile) != 2)
		assert(false);
	return r;
}

static int ReadS32()
{
	int r;
	if (fread(&r, 1, 4, gFile) != 4)
		assert(false);
	return r;
}

static float ReadR32()
{
	float r;
	if (fread(&r, 1, 4, gFile) != 4)
		assert(false);
	return r;
}

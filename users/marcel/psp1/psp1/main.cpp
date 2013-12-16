#include <ctrlsvc.h>
#include <displaysvc.h>
#include <kernel.h>
#include <libgu.h>
#include <libgum.h>
#include <malloc.h>
#include <psptypes.h>
#include <stdlib.h>
#include "DdsLoader.h"
#include "Exception.h"
#include "FileStream.h"
#include "Log.h"
#include "Mat4x4.h"
#include "SpriteGfx.h"
#include "SpriteRenderer.h"
#include "StreamReader.h"
#include "Types.h"

#include "tex_pf8888.h"

SCE_MODULE_INFO( psp1, 0, 1, 1 );

int sce_newlib_heap_kb_size = 4000; // 4 million bytes
char sce_user_main_thread_name[] = "main_thread";
//int sce_user_main_thread_priority = 32; // 64
unsigned int sce_user_main_thread_stack_kb_size = 256;

// GU display list

static char sCommandBuffer[0x10000] __attribute__((aligned(64)));
static ScePspFMatrix4 sMatrixStack[8 + 8 + 32 + 0];

// helpers

static inline uint32_t MakeColor(float r, float g, float b, float a)
{
	uint8_t ri = (uint8_t)(r * 255.0f);
	uint8_t gi = (uint8_t)(g * 255.0f);
	uint8_t bi = (uint8_t)(b * 255.0f);
	uint8_t ai = (uint8_t)(a * 255.0f);

	return (ri << 0) | (gi << 8) | (bi << 16) | (ai << 24);
}

// Log

static LogCtx mLog;

//

static void gfx_init()
{
	mLog.WriteLine(LogLevel_Debug, "Initializing PSP graphics unit");

	if (sceGuInit() < 0)
		throw ExceptionVA("unable to initialize GU");
	
	memset(sCommandBuffer, 0, sizeof(sCommandBuffer));

	// setup packet buffer

	if (sceGuStart(SCEGU_IMMEDIATE, sCommandBuffer, sizeof(sCommandBuffer)) < 0)
		throw ExceptionVA("unable to start GU display list");

	//if (sceDisplaySetMode(SCE_DISPLAY_MODE_LCD, SCEGU_SCR_WIDTH, SCEGU_SCR_HEIGHT) != 0)
	//	throw ExceptionVA("unable to set display mode");

	// setup front & back buffers

	sceGuDrawBuffer(SCEGU_PF5551, SCEGU_VRAM_BP_0, SCEGU_VRAM_WIDTH);
	sceGuDispBuffer(SCEGU_SCR_WIDTH, SCEGU_SCR_HEIGHT, SCEGU_VRAM_BP_1, SCEGU_VRAM_WIDTH);
	sceGuDepthBuffer(SCEGU_VRAM_BP_2, SCEGU_VRAM_WIDTH);

	// set viewport size

	sceGuOffset(SCEGU_SCR_OFFSETX, SCEGU_SCR_OFFSETY);
	sceGuViewport(2048, 2048, SCEGU_SCR_WIDTH, SCEGU_SCR_HEIGHT);
	//sceGuDepthRange(50000, 10000);

	sceGuScissor(0, 0, SCEGU_SCR_WIDTH, SCEGU_SCR_HEIGHT);
	sceGuEnable(SCEGU_SCISSOR_TEST);
	sceGuEnable(SCEGU_CLIP_PLANE);

	// initialize matrix stack

	sceGumSetMatrixStack(&sMatrixStack[0], 8, 8, 32, 0);

	// enable display

	sceGuDisplay(SCEGU_DISPLAY_ON);

	// process command list

	sceGuFinish();
	sceGuSync(SCEGU_SYNC_FINISH, SCEGU_SYNC_WAIT);
	sceDisplayWaitVblankStart();

	mLog.WriteLine(LogLevel_Debug, "Initializing PSP graphics unit [done]");
}

static void gfx_shut()
{
	mLog.WriteLine(LogLevel_Debug, "Shutting down PSP graphics unit");

	if (sceGuTerm() < 0)
		throw ExceptionVA("unable to shutdown GU");

	mLog.WriteLine(LogLevel_Debug, "Shutting down PSP graphics unit [done]");
}

static void gfx_makecurrent()
{
	mLog.WriteLine(LogLevel_Debug, "MakeCurrent");

	// setup packet buffer

	//memset(sCommandBuffer, 0, sizeof(sCommandBuffer));
	//memset(&sMatrixStack[0], 0, sizeof(sMatrixStack));

	if (sceGuStart(SCEGU_IMMEDIATE, sCommandBuffer, sizeof(sCommandBuffer)) < 0)
		throw ExceptionVA("unable to start GU display list");

	// set default states

	sceGuEnable(SCEGU_DITHER);

	sceGuDisable(SCEGU_DEPTH_TEST);
	//sceGuDisable(SCEGU_SCISSOR_TEST);
	sceGuDisable(SCEGU_ALPHA_TEST);
	sceGuDisable(SCEGU_BLEND);
	sceGuDisable(SCEGU_LIGHTING);
	sceGuDisable(SCEGU_CULL_FACE);
	sceGuDisable(SCEGU_TEXTURE);

	//sceGuTexFilter(SCEGU_LINEAR, SCEGU_LINEAR);
	sceGuShadeModel(SCEGU_SMOOTH);

	// initialize matrix stack

	sceGumSetMatrixStack(&sMatrixStack[0], 8, 8, 32, 0);

	static int x = 0;
	x++;
	sceGuFrontFace((x%2) ? SCEGU_CW : SCEGU_CCW);

	mLog.WriteLine(LogLevel_Debug, "MakeCurrent [done]");
}

static void gfx_clear(float r, float g, float b, float a)
{
	sceGuClearColor(MakeColor(r, g, b, a));
	sceGuClear(SCEGU_CLEAR_COLOR | SCEGU_CLEAR_FAST);
}

static void gfx_present()
{
	sceGuFinish();
	sceGuSync(SCEGU_SYNC_FINISH, SCEGU_SYNC_WAIT);
	sceDisplayWaitVblankStart();

	sceGuSwapBuffers();
}

static float Random(float min, float max)
{
	return (rand() & 4095) / 4095.0f * (max - min) + min;
}

#define SIZE 300

typedef struct
{
	float u, v;
	unsigned int   rgba;
	ScePspFVector3 vector;
} MODEL;
static const MODEL model[] = {
//  rgba          position
	{ 0.0f, 0.0f, 0xffffffff, {0.0f, 0.0f, 0.0f }},
	{ 1.0f, 0.0f, 0xff00ffff, {+SIZE, 0.0f, 0.0f }},
	{ 1.0f, 1.0f, 0xffff00ff, {+SIZE, +SIZE, 0.0f }},
	{ 1.0f, 1.0f, 0xffff00ff, {+SIZE, +SIZE, 0.0f }},
	{ 0.0f, 1.0f, 0xffff00ff, {0.0f, +SIZE, 0.0f }},
	{ 0.0f, 0.0f, 0xffffffff, {0.0f, 0.0f, 0.0f }}
};

static void MatrixMul(Mat4x4 m, bool overwrite)
{
	//m = m.CalcTranspose();

	ScePspFMatrix4 m2;

	m2.x.x = m.m_v[0];
	m2.x.y = m.m_v[1];
	m2.x.z = m.m_v[2];
	m2.x.w = m.m_v[3];

	m2.y.x = m.m_v[4];
	m2.y.y = m.m_v[5];
	m2.y.z = m.m_v[6];
	m2.y.w = m.m_v[7];

	m2.z.x = m.m_v[8];
	m2.z.y = m.m_v[9];
	m2.z.z = m.m_v[10];
	m2.z.w = m.m_v[11];

	m2.w.x = m.m_v[12];
	m2.w.y = m.m_v[13];
	m2.w.z = m.m_v[14];
	m2.w.w = m.m_v[15];

	if (overwrite)
		sceGumLoadMatrix(&m2);
	else
		sceGumMultMatrix(&m2);
}

void RenderSprite(SpriteGfx& gfx)
{
	gfx.Reserve(4, 6);
	gfx.WriteBegin();
	gfx.WriteVertex(0.0f, 0.0f, SpriteColor_Make(255, 0, 0, 255).rgba, 0.0f, 1.0f);
	gfx.WriteVertex(100.0f, 0.0f, SpriteColor_Make(255, 255, 0, 255).rgba, 1.0f, 1.0f);
	gfx.WriteVertex(100.0f, 100.0f, SpriteColor_Make(255, 0, 255, 255).rgba, 1.0f, 0.0f);
	gfx.WriteVertex(0.0f, 100.0f, SpriteColor_Make(255, 255, 255, 255).rgba, 0.0f, 1.0f);
	gfx.WriteIndex3(0, 1, 2);
	gfx.WriteIndex3(0, 2, 3);
	gfx.WriteEnd();
}

static bool sTextureIsInitialized = false;
static const int sTextureSx = 64;
static const int sTextureSy = 64;
static const int sTextureByteCount = sTextureSx * sTextureSy * 4;
static uint8_t sTextureBytes[sTextureByteCount] __attribute__((aligned(16)));

static void MakeFast(void* bytes, int sx, int sy, int format, int byteCount)
{
	Assert(format == SCEGU_PF8888);

	uint8_t* temp = new uint8_t[byteCount];

	sceGuCopyImage2(
		format,
		0, 0, sx, sy, sx, bytes,
		0, 0, sx, temp,
		0, 0, 
		SCEGU_TEXBUF_NORMAL, SCEGU_TEXBUF_FAST);

	sceGuFinish();
	sceGuSync(SCEGU_SYNC_FINISH, SCEGU_SYNC_WAIT);

	memcpy(bytes, temp, byteCount);

	delete[] temp;

	gfx_makecurrent();
}

static void SetTextureEx(int format, const void* bytes, int sx, int sy)
{
	// fast mode 32 bpp:
	//
	// 00 01 02 03 32 33 34 35
	// 04 05 06 07 36 37 38 ..
	// 08 09 10 11 .. ..
	// 12 13 14 15
	// 16 17 18 19
	// 20 21 22 23
	// 24 25 26 27
	// 28 29 30 31

	//int mode = SCEGU_TEXBUF_FAST;
	int mode = SCEGU_TEXBUF_NORMAL;

	sceGuTexMode(
		format,
		0,
		SCEGU_SINGLE_CLUT,
		mode);

	sceGuTexImage(
		0,
		sx,
		sy,
		sx,
		bytes);

	sceGuTexFunc(SCEGU_TEX_REPLACE, SCEGU_RGBA);
	sceGuTexWrap(SCEGU_REPEAT, SCEGU_REPEAT);
	sceGuTexScale(1.0f, 1.0f);
	sceGuTexOffset(0.0f, 0.0f);
	//sceGuTexMapMode(SCEGU_UV_MAP, 0, 1);
	//sceGuTexFilter(SCEGU_NEAREST, SCEGU_NEAREST); 
	sceGuTexFilter(SCEGU_LINEAR, SCEGU_LINEAR); 

	sceGuEnable(SCEGU_TEXTURE);

	sceGuTexFlush();
}

static void SetTexture()
{
	if (sTextureIsInitialized == false)
	{
		sTextureIsInitialized = true;
		int f[2] = { 0, 1 };
		for (int i = 0; i < sTextureByteCount; ++i)
		{
			int c = (f[0] + f[1]) * 33;
			f[0] = f[1];
			f[1] = c;
			//sTextureBytes[i] = rand();
			//sTextureBytes[i] = c & 255;
			sTextureBytes[i] = i * 255 / sTextureByteCount;
		}
		sceKernelDcacheWritebackAll();
		MakeFast(sTextureBytes, sTextureSx, sTextureSy, SCEGU_PF8888, sTextureByteCount);
	}

	SetTextureEx(SCEGU_PF8888, sTextureBytes, sTextureSx, sTextureSy);
}

static void SetTexture2()
{
	SetTextureEx(SCEGU_PF8888, pix_tex_pf8888, 256, 128);
}

class TexInfo
{
public:
	TexInfo(int format, void* bytes, int sx, int sy)
	{
		this->format = format;
		this->bytes = bytes;
		this->sx = sx;
		this->sy = sy;
	}

	int format;
	void* bytes;
	int sx;
	int sy;
};

static void SetTexture(TexInfo* tex)
{
	SetTextureEx(tex->format, tex->bytes, tex->sx, tex->sy);
}

static void Swizzle(void* p, int byteCount, int blockSize)
{
	uint32_t* p32 = (uint32_t*)p;
	int blockCount = byteCount / blockSize;
	for (int i = 0; i < blockCount / 2; ++i)
	{
		// 64 bit data
		//p32[i * 2] = 0;
		std::swap(p32[i * 2 + 0], p32[i * 2 + 1]);
	}
#if 0
		int i16count = byteCount / 2;
		for (int i = 0; i < i16count; ++i)
		{
			uint8_t* p = &((uint8_t*)bytes)[i * 2];
			std::swap(p[0], p[1]);
		}
#endif
}

TexInfo* LoadDDS(const char* fileName)
{
	FileStream stream(fileName, OpenMode_Read);
	StreamReader reader(&stream, false);
	DdsLoader loader;
	loader.LoadHeader(reader);
	stream.Seek(0, SeekMode_Begin);
	loader.SeekToData(0, reader);
	if (loader.IsFourCC("DXT1"))
	{
		int byteCount = loader.mHeader.dwWidth * loader.mHeader.dwHeight * 4 / 8;
		void* bytes = memalign(64, byteCount);
		int read = stream.Read(bytes, byteCount);
		if (read != byteCount)
			throw ExceptionNA();
		Swizzle(bytes, byteCount, 4);
		sceKernelDcacheWritebackAll();
		/*gfx_makecurrent();
		MakeFast(bytes, loader.mHeader.dwWidth, loader.mHeader.dwHeight, SCEGU_PFDXT1, byteCount);
		gfx_present();*/
		return new TexInfo(SCEGU_PFDXT1, bytes, loader.mHeader.dwWidth, loader.mHeader.dwHeight);
	}
	if (loader.IsFourCC("DXT3"))
	{
		int byteCount = loader.mHeader.dwWidth * loader.mHeader.dwHeight * 8 / 8;
		void* bytes = memalign(64, byteCount);
		int read = stream.Read(bytes, byteCount);
		if (read != byteCount)
			throw ExceptionNA();
		sceKernelDcacheWritebackAll();
		return new TexInfo(SCEGU_PFDXT3, bytes, loader.mHeader.dwWidth, loader.mHeader.dwHeight);
	}
	if (loader.IsFourCC("DXT5"))
	{
		int byteCount = loader.mHeader.dwWidth * loader.mHeader.dwHeight * 8 / 8;
		void* bytes = memalign(64, byteCount);
		int read = stream.Read(bytes, byteCount);
		if (read != byteCount)
			throw ExceptionNA();
		sceKernelDcacheWritebackAll();
		return new TexInfo(SCEGU_PFDXT5, bytes, loader.mHeader.dwWidth, loader.mHeader.dwHeight);
	}

	throw ExceptionNA();
}

int main(int argc, char* argv[])
{
	sceKernelSetCompiledSdkVersion(SCE_DEVKIT_VERSION);

	mLog = LogCtx("log");

	gfx_init();

	mLog.WriteLine(LogLevel_Debug, "screen size: %dx%d", SCEGU_SCR_WIDTH, SCEGU_SCR_HEIGHT);

	SpriteGfx gfx(100, 400, SpriteGfxRenderTime_OnFrameEnd);

	RenderSprite(gfx);

	TexInfo* tex = LoadDDS("host0:test1.dds");

	sceKernelDcacheWritebackAll();

	while (true)
	{
		gfx_makecurrent();
		gfx_clear(Random(0.0f, 1.0f), Random(0.0f, 0.5f), Random(0.0f, 0.25f), 0.0f);

		sceGumMatrixMode(SCEGU_MATRIX_PROJECTION);
		sceGumLoadIdentity();

#if 1
		Mat4x4 matP;
		matP.MakeOrthoLH(0.0f, SCEGU_SCR_WIDTH, 0.0f, SCEGU_SCR_HEIGHT, 1.0f, 200.0f);
		MatrixMul(matP, true);
#elif 0
		sceGumOrtho(0.0f, SCEGU_SCR_WIDTH, SCEGU_SCR_HEIGHT, 0.0f, 1.0f, 200.0f);
#else
		sceGumPerspective(
			SCEGU_RAD(90.0f),
			SCEGU_SCR_ASPECT,
			1.000000f,
			200.000000f);
#endif

		sceGumMatrixMode(SCEGU_MATRIX_VIEW);
		sceGumLoadIdentity();

		Mat4x4 matV;
		//matV.MakeLookat(Vec3(0.0f, 0.0f, 0.0f), Vec3(0.0f, 0.0f, 1.0f), Vec3(0.0f, -1.0f, 0.0f));
		matV.MakeIdentity();
		MatrixMul(matV, true);

#if 0
		ScePspFVector3 camPos;
		ScePspFVector3 camTgt;
		ScePspFVector3 camUp;

		camPos.x = 0.0f;
		camPos.y = 0.0f;
		camPos.z = 0.0f;
		camTgt.x = 0.0f;
		camTgt.y = 0.0f;
		camTgt.z = 1.0f;
		camUp.x = 0.0f;
		camUp.y = -1.0f;
		camUp.z = 0.0f;
		sceGumLookAt(
			&camPos,
			&camTgt,
			&camUp);
#endif

		sceGumMatrixMode(SCEGU_MATRIX_WORLD);
		sceGumLoadIdentity();
		Mat4x4 matW;
		matW.MakeIdentity();
		MatrixMul(matW, true);

		static float x = 0.0f;
		x += 10.0f / 60.0f;

		ScePspFVector3 p;
		p.x = x;
		p.y = 0.0f;
		p.z = 100.0f;
		sceGumTranslate(&p);

		//SetTexture();
		//SetTexture2();
		SetTexture(tex);

#if 1
		sceGumDrawArray(
			SCEGU_PRIM_TRIANGLES,
			SCEGU_TEXTURE_FLOAT |
			SCEGU_COLOR_PF8888 |
			SCEGU_VERTEX_FLOAT,
			6, NULL, model);
#else
		gfx.DBG_Render();
#endif

		sceGuColor(0xffffffff);
		sceGuDebugPrint(10, 10, 0x00ffffff, "hello world");

		gfx_present();
	}

	gfx_shut();

	return 0;
}

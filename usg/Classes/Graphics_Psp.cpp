#include <displaysvc.h>
#include <geman.h>
#include <kernel.h>
#include <libgu.h>
#include <libgum.h>
#include <string.h>
#include "Exception.h"
#include "Graphics.h"
#include "Graphics_Psp.h"
#include "TextureDDS.h"
#include "TextureRGBA.h"

#define SIGNAL_SWAP 100

// GU display list

//static char sCommandBuffer[0x100000] __attribute__((aligned(64))); // 1M
static char sCommandBuffer[0x40000] __attribute__((aligned(64))); // 256K
static ScePspFMatrix4 sMatrixStack[8 + 8 + 32 + 0];

// helpers

static void ConvertMatrix(const Mat4x4& m, ScePspFMatrix4& m2);
static void MatrixActivate(MatrixType type);
//static void MatrixMultiply(const Mat4x4& m);
static void MatrixLoad(const Mat4x4& m);

static SceUID sBufferSwapSema = 0;
static SceUID sBufferSwapCompleteSema = 0;
static SceUID sBufferSwapThreadId = 0;
static bool sBufferSwapThreadExit = false;

static LogCtx sLog("Graphics_Psp");

static int BufferSwapThread(SceSize, void*)
{
	sLog.WriteLineNA(LogLevel_Info, "Started buffer swap thread");

	while (sBufferSwapThreadExit == false)
	{
		sceKernelWaitSema(sBufferSwapSema, 1, 0);

		sceDisplayWaitVblank();

		sceGuSwapBuffers();

		sceKernelSignalSema(sBufferSwapCompleteSema, 1);

		//sLog.WriteLineNA(LogLevel_Debug, "Swap!");
	}

	sLog.WriteLineNA(LogLevel_Info, "Exited buffer swap thread");

	return 0;
}

static void BufferSwapSignal(int id)
{
	sceKernelSignalSema(sBufferSwapSema, 1);
}

class TextureData
{
public:
	TextureData(int pixelFormat, int mode, int sx, int sy, uint8_t* bytes, bool ownBytes)
	{
		if (sx <= 0 || sy <= 0)
		{
#ifndef DEPLOYMENT
			throw ExceptionVA("invalid size: %dx%d", sx, sy);
#endif
		}
		if (bytes == 0)
		{
#ifndef DEPLOYMENT
			throw ExceptionVA("byte array not set");
#endif
		}

		mPixelFormat = pixelFormat;
		mMode = mode;
		mSx = sx;
		mSy = sy;
		mBytes = bytes;
		mOwnBytes = ownBytes;
	}

	~TextureData()
	{
		if (mOwnBytes)
		{
			delete[] mBytes;
			mBytes = 0;
		}
	}

	int mPixelFormat;
	int mMode;
	int mSx;
	int mSy;
	uint8_t* mBytes;
	bool mOwnBytes;
};

static inline uint32_t MakeColor(float r, float g, float b, float a)
{
	uint8_t ri = (uint8_t)(r * 255.0f);
	uint8_t gi = (uint8_t)(g * 255.0f);
	uint8_t bi = (uint8_t)(b * 255.0f);
	uint8_t ai = (uint8_t)(a * 255.0f);

	return (ri << 0) | (gi << 8) | (bi << 16) | (ai << 24);
}

static inline void SetViewport()
{
	// set viewport size / offset

	sceGuOffset(SCEGU_SCR_OFFSETX, SCEGU_SCR_OFFSETY);
	sceGuViewport(2048, 2048, SCEGU_SCR_WIDTH, SCEGU_SCR_HEIGHT);
	//sceGuDepthRange(50000, 10000);
	//sceGuDepthRange(30000, -30000);
}

//

Graphics_Psp::Graphics_Psp()
{
	mLog = LogCtx("Graphics_Psp");

	mRenderInProgress = false;
	mAsyncSwapEnabled = true;
}

Graphics_Psp::~Graphics_Psp()
{
}

void Graphics_Psp::Initialize(int sx, int sy)
{
	mLog.WriteLine(LogLevel_Debug, "Initializing PSP graphics unit");

	if (sx <= 0 || sy <= 0)
		throw ExceptionVA("invalid size: %dx%d", sx, sy);

	//

	if (sceGuInit() < 0)
		throw ExceptionVA("unable to initialize GU");
	
	//

	const int vramOffset = ((uint8_t*)SCEGU_VRAM_BP_2) - ((uint8_t*)SCEGU_VRAM_TOP);

	void* vramBytes = ((uint8_t*)sceGeEdramGetAddr()) + vramOffset;

	mVramAllocator.Setup(vramBytes);

	// setup packet buffer

	if (sceGuStart(SCEGU_IMMEDIATE, sCommandBuffer, sizeof(sCommandBuffer)) < 0)
	{
#ifndef DEPLOYMENT
		throw ExceptionVA("unable to start GU display list");
#endif
	}

	//

	sBufferSwapSema = sceKernelCreateSema("BufferSwapSema", SCE_KERNEL_SA_THFIFO, 0, 1, 0);
	if (sBufferSwapSema <= 0)
		throw ExceptionVA("unable to create buffer swap sema");
	sBufferSwapCompleteSema = sceKernelCreateSema("BufferSwapDoneSema", SCE_KERNEL_SA_THFIFO, 1, 1, 0);
	if (sBufferSwapCompleteSema <= 0)
		throw ExceptionVA("unable to create buffer swap complete sema");
	sBufferSwapThreadId = sceKernelCreateThread("BufferSwapThread", BufferSwapThread, SCE_KERNEL_USER_HIGHEST_PRIORITY, 1024, 0, 0);
	if (sBufferSwapThreadId <= 0)
		throw ExceptionVA("unable to create buffer swap thread");
	if (sceKernelStartThread(sBufferSwapThreadId, 0, 0) < 0)
		throw ExceptionVA("unable to start buffer swap thread");

	sceGuSetCallback(SCEGU_INT_FINISH, BufferSwapSignal);
	//sceGuSetCallback(SCEGU_INT_SIGNAL, BufferSwapSignal);

	//

	mRenderInProgress = true;

	// setup front & back buffers

	sceGuDrawBuffer(SCEGU_PF5650, SCEGU_VRAM_BP_0, SCEGU_VRAM_WIDTH);
	sceGuDispBuffer(SCEGU_SCR_WIDTH, SCEGU_SCR_HEIGHT, SCEGU_VRAM_BP_1, SCEGU_VRAM_WIDTH);
	//sceGuDepthBuffer(SCEGU_VRAM_BP_2, SCEGU_VRAM_WIDTH);

	// viewport

	SetViewport();

	// clipping

	sceGuScissor(0, 0, SCEGU_SCR_WIDTH, SCEGU_SCR_HEIGHT);
	sceGuEnable(SCEGU_SCISSOR_TEST);
	sceGuEnable(SCEGU_CLIP_PLANE);

	// initialize matrix stack

	sceGumSetMatrixStack(&sMatrixStack[0], 8, 8, 32, 0);

	// enable display

	sceGuDisplay(SCEGU_DISPLAY_ON);

	// finish drawing

	Finish(false);

	mLog.WriteLine(LogLevel_Debug, "Initializing PSP graphics unit [done]");
}

void Graphics_Psp::Shutdown()
{
	mLog.WriteLine(LogLevel_Debug, "Shutting down PSP graphics unit");

	// destroy buffer swap thread

	sBufferSwapThreadExit = true;
	sceKernelSignalSema(sBufferSwapSema, 1);
	sceKernelWaitThreadEnd(sBufferSwapThreadId, 0);
	sceKernelDeleteThread(sBufferSwapThreadId);
	sBufferSwapThreadId = 0;

	//

	if (sceGuTerm() < 0)
		throw ExceptionVA("unable to shutdown GU");

	mLog.WriteLine(LogLevel_Debug, "Shutting down PSP graphics unit [done]");
}

void Graphics_Psp::MakeCurrent()
{
	sceKernelWaitSema(sBufferSwapCompleteSema, 1, 0);

	//mLog.WriteLine(LogLevel_Debug, "MakeCurrent");

	if (mRenderInProgress == true)
	{
#ifndef DEPLOYMENT
		throw ExceptionVA("render already in progress");
#else
		return;
#endif
	}

	// setup packet buffer

	int ret = sceGuStart(SCEGU_IMMEDIATE, sCommandBuffer, sizeof(sCommandBuffer));

	if (ret < 0)
	{
#ifndef DEPLOYMENT
		throw ExceptionVA("unable to start GU display list: 0x%08x", ret);
#endif
	}

	mRenderInProgress = true;

	// viewport

	SetViewport();

	// initialize matrix stack

	sceGumSetMatrixStack(&sMatrixStack[0], 8, 8, 32, 0);

	// set default states

	sceGuEnable(SCEGU_DITHER);

	sceGuDisable(SCEGU_DEPTH_TEST);
	sceGuDisable(SCEGU_SCISSOR_TEST);
	sceGuDisable(SCEGU_CLIP_PLANE);
	sceGuDisable(SCEGU_ALPHA_TEST);
	sceGuDisable(SCEGU_BLEND);
	sceGuDisable(SCEGU_LIGHTING);
	sceGuDisable(SCEGU_CULL_FACE);
	sceGuDisable(SCEGU_TEXTURE);

	sceGuTexFilter(SCEGU_LINEAR, SCEGU_LINEAR);
	sceGuShadeModel(SCEGU_SMOOTH);

	BlendModeSet(BlendMode_Normal_Opaque);

	sceGumMatrixMode(SCEGU_MATRIX_PROJECTION);
	sceGumLoadIdentity();
	sceGumMatrixMode(SCEGU_MATRIX_VIEW);
	sceGumLoadIdentity();
	sceGumMatrixMode(SCEGU_MATRIX_WORLD);
	sceGumLoadIdentity();

	//mLog.WriteLine(LogLevel_Debug, "MakeCurrent [done]");
}

void Graphics_Psp::Clear(float r, float g, float b, float a)
{
	if (mRenderInProgress == false)
	{
#ifndef DEPLOYMENT
		throw ExceptionVA("no render in progress");
#else
		return;
#endif
	}

	sceGuClearColor(MakeColor(r, g, b, a));
	sceGuClear(SCEGU_CLEAR_COLOR | SCEGU_CLEAR_FAST);
}

void Graphics_Psp::BlendModeSet(BlendMode mode)
{
	if (mRenderInProgress == false)
	{
#ifndef DEPLOYMENT
		throw ExceptionVA("no render in progress");
#else
		return;
#endif
	}

	switch (mode)
	{
	case BlendMode_Additive:
		sceGuEnable(SCEGU_TEXTURE);
		sceGuEnable(SCEGU_BLEND);
		sceGuBlendFunc(SCEGU_ADD, SCEGU_SRC_ALPHA, SCEGU_FIX_VALUE, 0, 0xFFFFFFFF);
		sceGuTexFunc(SCEGU_TEX_MODULATE, SCEGU_RGBA);
		break;
		
	case BlendMode_Normal_Opaque:
		sceGuEnable(SCEGU_TEXTURE);
		sceGuDisable(SCEGU_BLEND);
		sceGuBlendFunc(SCEGU_ADD, SCEGU_SRC_ALPHA, SCEGU_ONE_MINUS_SRC_ALPHA, 0, 0);
		sceGuTexFunc(SCEGU_TEX_MODULATE, SCEGU_RGBA);
		break;

	case BlendMode_Normal_Opaque_Add:
		sceGuEnable(SCEGU_TEXTURE);
		sceGuDisable(SCEGU_BLEND);
		sceGuBlendFunc(SCEGU_ADD, SCEGU_SRC_ALPHA, SCEGU_ONE_MINUS_SRC_ALPHA, 0, 0);
		sceGuTexFunc(SCEGU_TEX_ADD, SCEGU_RGBA);
		break;
		
	case BlendMode_Normal_Transparent:
		sceGuEnable(SCEGU_TEXTURE);
		sceGuEnable(SCEGU_BLEND);
		sceGuBlendFunc(SCEGU_ADD, SCEGU_SRC_ALPHA, SCEGU_ONE_MINUS_SRC_ALPHA, 0, 0);
		sceGuTexFunc(SCEGU_TEX_MODULATE, SCEGU_RGBA);
		break;
		
	case BlendMode_Normal_Transparent_Add:
		sceGuEnable(SCEGU_TEXTURE);
		sceGuEnable(SCEGU_BLEND);
		sceGuBlendFunc(SCEGU_ADD, SCEGU_SRC_ALPHA, SCEGU_ONE_MINUS_SRC_ALPHA, 0, 0);
		sceGuTexFunc(SCEGU_TEX_ADD, SCEGU_RGBA);
		break;
		
	case BlendMode_Subtractive:
		sceGuEnable(SCEGU_TEXTURE);
		sceGuEnable(SCEGU_BLEND);
		sceGuBlendFunc(SCEGU_REVERSE_SUBTRACT, SCEGU_SRC_ALPHA, SCEGU_FIX_VALUE, 0, 0xFFFFFFFF);
		sceGuTexFunc(SCEGU_TEX_MODULATE, SCEGU_RGBA);
		break;
		
	default:
#ifndef DEPLOYMENT
		throw ExceptionNA();
#else
		break;
#endif
	}
}

void Graphics_Psp::MatrixSet(MatrixType type, const Mat4x4& mat)
{
	if (mRenderInProgress == false)
	{
#ifndef DEPLOYMENT
		throw ExceptionVA("no render in progress");
#else
		return;
#endif
	}

	MatrixActivate(type);
	MatrixLoad(mat);
}

void Graphics_Psp::MatrixPush(MatrixType type)
{
	if (mRenderInProgress == false)
	{
#ifndef DEPLOYMENT
		throw ExceptionVA("no render in progress");
#else
		return;
#endif
	}

	MatrixActivate(type);
	sceGumPushMatrix();
}

void Graphics_Psp::MatrixPop(MatrixType type)
{
	if (mRenderInProgress == false)
	{
#ifndef DEPLOYMENT
		throw ExceptionVA("no render in progress");
#else
		return;
#endif
	}

	MatrixActivate(type);
	sceGumPopMatrix();
}

void Graphics_Psp::MatrixTranslate(MatrixType type, float x, float y, float z)
{
	if (mRenderInProgress == false)
	{
#ifndef DEPLOYMENT
		throw ExceptionVA("no render in progress");
#else
		return;
#endif
	}

	MatrixActivate(type);

	ScePspFVector3 v;
	v.x = x;
	v.y = y;
	v.z = z;

	sceGumTranslate(&v);
}

void Graphics_Psp::MatrixRotate(MatrixType type, float angle, float x, float y, float z)
{
	if (mRenderInProgress == false)
	{
#ifndef DEPLOYMENT
		throw ExceptionVA("no render in progress");
#else
		return;
#endif
	}

	MatrixActivate(type);

	if (x == 1.0f)
		sceGumRotateX(angle);
	else if (y == 1.0f)
		sceGumRotateY(angle);
	else if (z == 1.0f)
		sceGumRotateZ(angle);
	else
	{
#if !defined(DEPLOYMENT)
		throw ExceptionVA("matrix rotation around arbitrary axis not implemented");
#endif
	}
}

void Graphics_Psp::MatrixScale(MatrixType type, float x, float y, float z)
{
	if (mRenderInProgress == false)
	{
#ifndef DEPLOYMENT
		throw ExceptionVA("no render in progress");
#else
		return;
#endif
	}

	MatrixActivate(type);

	ScePspFVector3 v;
	v.x = x;
	v.y = y;
	v.z = z;

	sceGumScale(&v);
}

void Graphics_Psp::TextureSet(Res* texture)
{
	if (mRenderInProgress == false)
	{
#ifndef DEPLOYMENT
		throw ExceptionVA("no render in progress");
#else
		return;
#endif
	}

	// disable texture mapping if texture is 0

	if (texture == 0)
	{
		sceGuDisable(SCEGU_TEXTURE);
		return;
	}

#if 0
	// cache texture, if it's not already cached

	if (texture->DeviceData_get() == 0)
	{
		TextureCreate(texture);
	}
#endif

	TextureData* data = (TextureData*)texture->DeviceData_get();

	if (data == 0)
	{
#if !defined(DEPLOYMENT)
		mLog.WriteLine(LogLevel_Error, "Device data not set");
#endif
		sceGuDisable(SCEGU_TEXTURE);
		return;
	}

	Assert(data->mSx > 0);
	Assert(data->mSy > 0);
	Assert(data->mBytes != 0);

	sceGuTexMode(
		data->mPixelFormat,
		0,
		SCEGU_SINGLE_CLUT,
		data->mMode);

	sceGuTexImage(
		0,
		data->mSx,
		data->mSy,
		data->mSx,
		data->mBytes);

	sceGuEnable(SCEGU_TEXTURE);

	sceGuTexFlush();
}

void Graphics_Psp::TextureIsEnabledSet(bool value)
{
	if (value)
		sceGuEnable(SCEGU_TEXTURE);
	else
		sceGuDisable(SCEGU_TEXTURE);
}

void Graphics_Psp::TextureCreate(Res* res)
{
	//if (mRenderInProgress == true)
	//	throw ExceptionVA("render in progress, cannot cache texture");
	if (res->DeviceData_get() != 0)
	{
#ifndef DEPLOYMENT
		throw ExceptionVA("texture already cached");
#else
		return;
#endif
	}

	bool isInRender = mRenderInProgress;

	if (isInRender)
	{
		mLog.WriteLine(LogLevel_Warning, "Already in render. finishing current render");
		Finish(true);
	}

	if (res->m_Type == ResTypes_TextureDDS)
	{
		mLog.WriteLine(LogLevel_Debug, "Creating cached texture (DDS)");

		res->Open();

		// make sure texture data is flushed to main memory if stored in D cache

		sceKernelDcacheWritebackAll();

		//

		TextureDDS* texture = (TextureDDS*)res->Data_get();

		if (texture == 0)
			throw ExceptionVA("texture data is 0");

		const int sx = texture->mSx;
		const int sy = texture->mSy;
		const int byteCount = texture->mByteCount;

		Assert(sx > 0);
		Assert(sy > 0);
		Assert(byteCount > 0);

		// note: DXT data is assumed to be already 'fixed' for PSP target

		int mode = -1;

		switch (texture->mType)
		{
		case TextureDDSType_DXT1:
			mode = SCEGU_PFDXT1;
			break;
		case TextureDDSType_DXT3:
			mode = SCEGU_PFDXT3;
			break;
		case TextureDDSType_DXT5:
			mode = SCEGU_PFDXT5;
			break;
		default:
#ifndef DEPLOYMENT
			throw ExceptionVA("unknown texture mode: %s", (int)texture->mType);
#else
			mode = SCEGU_PFDXT1;
			break;
#endif
		}

#if 0
		uint8_t* bytes = new uint8_t[byteCount];

		memcpy(bytes, texture->mBytes, byteCount);

		TextureData* data = new TextureData(mode, SCEGU_TEXBUF_NORMAL, sx, sy, bytes, true);
#else
		// move DDS textures int VRAM

		mLog.WriteLine(LogLevel_Debug, "Uploading DDS texture to VRAM");

		uint8_t* bytes = (uint8_t*)mVramAllocator.Alloc(byteCount);

		MakeCurrent();

		sceGuCopyImage2(
			mode,
			0, 0, sx, sy, sx, texture->mBytes,
			0, 0, sx, bytes,
			0, 0,
			SCEGU_TEXBUF_NORMAL, SCEGU_TEXBUF_NORMAL);

		Finish(false);

		mLog.WriteLine(LogLevel_Debug, "Uploading DDS texture to VRAM [done]");

		TextureData* data = new TextureData(mode, SCEGU_TEXBUF_NORMAL, sx, sy, bytes, false);
#endif

		res->DeviceData_set(data);

		res->Close();
	}
	else if (res->m_Type == ResTypes_TextureRGBA)
	{
		mLog.WriteLine(LogLevel_Debug, "Creating cached texture (RGBA)");

		res->Open();

		// make sure texture data is flushed to main memory if stored in D cache

		sceKernelDcacheWritebackAll();

		//

		TextureRGBA* texture = (TextureRGBA*)res->Data_get();

		if (texture == 0)
			throw ExceptionVA("texture data is 0");

		const int sx = texture->Sx_get();
		const int sy = texture->Sy_get();

		Assert(sx > 0);
		Assert(sy > 0);

		// note: RGBA data is assumed to be already 'fixed' for PSP target

		uint8_t* bytes = texture->Bytes_get();

		texture->DisownBytes();

		TextureData* data = new TextureData(SCEGU_PF8888, SCEGU_TEXBUF_FAST, sx, sy, bytes, true);

		res->DeviceData_set(data);

		res->Close();
	}
	else if (res->m_Type == ResTypes_TexturePVR)
	{
		throw ExceptionVA("PVR texture type not supported on PSP");
	}
	else
	{
		throw ExceptionVA("unknown texture type: %d", (int)res->m_Type);
	}

	if (isInRender)
		MakeCurrent();
}

void Graphics_Psp::TextureDestroy(Res* res)
{
	// not cached?

	if (res->DeviceData_get() == 0)
		return;

	if (res->m_Type == ResTypes_TextureDDS || res->m_Type == ResTypes_TextureRGBA)
	{
		mLog.WriteLine(LogLevel_Debug, "Destroying cached texture");

		//DBG_PrintAllocState();

		TextureData* data = (TextureData*)res->DeviceData_get();

		if (res->m_Type == ResTypes_TextureDDS)
		{
			mVramAllocator.Free(data->mBytes);
		}

		delete data;
		data = 0;

		res->DeviceData_set(0);

		//DBG_PrintAllocState();
	}
	else if (res->m_Type == ResTypes_TexturePVR)
	{
		throw ExceptionVA("PVR texture type not supported on PSP");
	}
	else
	{
		throw ExceptionVA("unknown texture type: %d", (int)res->m_Type);
	}
}

void Graphics_Psp::Present()
{
	if (mRenderInProgress == false)
	{
#ifndef DEPLOYMENT
		throw ExceptionVA("no render in progress. cannot finish");
#else
		return;
#endif
	}

	// finish drawing

	Finish(true);
}

void Graphics_Psp::FinishAndRestart()
{
	if (mRenderInProgress == false)
	{
#ifndef DEPLOYMENT
		throw ExceptionVA("no render in progress. cannot finish and restart");
#endif
	}

	Finish(false);

	// setup packet buffer

	if (sceGuStart(SCEGU_IMMEDIATE, sCommandBuffer, sizeof(sCommandBuffer)) < 0)
		throw ExceptionVA("unable to start GU display list");

	mRenderInProgress = true;
}

void Graphics_Psp::Finish(bool asyncSwap)
{
	if (mRenderInProgress == false)
	{
#ifndef DEPLOYMENT
		throw ExceptionVA("no render in progress. cannot finish");
#else
		return;
#endif
	}

	// add display list end command
	if (asyncSwap == true)
	{
		//sceGuSignal(SCEGU_SIGNAL_NOWAIT, SIGNAL_SWAP);
		//sceGuSignalSync(SIGNAL_SWAP);
	}

	sceGuFinish();

	// if not async, wait for the GPU to finish
	if (asyncSwap == false)
	{
		LOG_WRN("GU sync", 0);
		//sceGuFlush();
		sceGuSync(SCEGU_SYNC_FINISH, SCEGU_SYNC_WAIT);
	}

	mRenderInProgress = false;
}

void Graphics_Psp::ManualSwap()
{
	sceKernelSignalSema(sBufferSwapSema, 1);
}

void Graphics_Psp::Block()
{
	if (mRenderInProgress)
	{
#ifndef DEPLOYMENT
		throw ExceptionNA();
#else
		return;
#endif
	}

	sceKernelWaitSema(sBufferSwapCompleteSema, 1, 0);

	sceKernelSignalSema(sBufferSwapCompleteSema, 1);
}

void Graphics_Psp::AsyncSwapEnabled_set(bool enabled)
{
	if (enabled == mAsyncSwapEnabled)
		return;

	Block();

	mAsyncSwapEnabled = enabled;

	if (enabled)
		sceGuSetCallback(SCEGU_INT_FINISH, BufferSwapSignal);
	else
		sceGuSetCallback(SCEGU_INT_FINISH, 0);
}

static void ConvertMatrix(const Mat4x4& m, ScePspFMatrix4& m2)
{
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
}

static void MatrixActivate(MatrixType type)
{
	switch (type)
	{
	case MatrixType_Projection:
		sceGumMatrixMode(SCEGU_MATRIX_PROJECTION);
		break;
	case MatrixType_World:
		sceGumMatrixMode(SCEGU_MATRIX_WORLD);
		break;

	default:
#if !defined(DEPLOYMENT)
		throw ExceptionNA();
#else
		break;
#endif
	}
}

/*static void MatrixMultiply(const Mat4x4& m)
{
	ScePspFMatrix4 m2;
	
	ConvertMatrix(m, m2);
	sceGumMultMatrix(&m2);
}*/

static void MatrixLoad(const Mat4x4& m)
{
	ScePspFMatrix4 m2;
	
	ConvertMatrix(m, m2);

	sceGumLoadMatrix(&m2);
}

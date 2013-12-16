#include "Benchmark.h"
#include "Calc.h"
#include "OpenGLCompat.h"
#include "OpenGLUtil.h"
#include "TexturePVR.h"
#include "TextureRGBA.h"

#define USE_RGBA8_TEX 1
#define USE_MIPMAPS 0

LogCtx OpenGLUtil::m_Log;

#ifdef IPHONEOS
static void UploadPvr_Iphone(const TexturePVR* texture);
#endif

void OpenGLUtil::SetTexture(IRes* texture)
{
	if (!texture->DeviceData_get())
		CreateTexture(texture);
	
	GLuint* textureId = (GLuint*)texture->DeviceData_get();
	Assert(textureId != 0);
	
	glBindTexture(GL_TEXTURE_2D, *textureId);
}

#if USE_RGBA8_TEX == 0

static uint16_t* To16BPP(TextureRGBA* texture)
{
	const int area = texture->Sx_get() * texture->Sy_get();
	
	uint16_t* result = new uint16_t[area];

	const uint8_t* srcBytes = texture->Bytes_get();
	uint16_t* dstBytes = result;
	
	for (int i = 0; i < area; ++i)
	{
		const int r = srcBytes[0] >> 3;
		const int g = srcBytes[1] >> 2;
		const int b = srcBytes[2] >> 3;
		
		const uint16_t color = (r << 11) | (g << 5) | (b << 0);
		
		*dstBytes++ = color;
		
		srcBytes += 4;
	}
	
	return result;
}

#endif

void OpenGLUtil::CreateTexture(IRes* texture)
{
	UsingBegin(Benchmark bm("OpenGL.CreateTexture"))
	{
		m_Log.WriteLine(LogLevel_Info, "Creating texture");
		
		if (!texture->IsType("tex_pvr") && !texture->IsType("tex_rgba"))
			throw ExceptionVA("unsupported resource type");
		
		GLuint* textureId = new GLuint;
		
		glGenTextures(1, textureId);
		glBindTexture(GL_TEXTURE_2D, *textureId);

		if (texture->IsType("tex_pvr"))
		{
			m_Log.WriteLine(LogLevel_Info, "Uploading PVR texture");
			
#ifdef IPHONEOS
			texture->Open();

			const TexturePVR* texturePVR = (TexturePVR*)texture->Data_get();

			UploadPvr_Iphone(texturePVR);

			texture->Close();
#elif defined(WIN32) || defined(LINUX) || defined(MACOS) || defined(BBOS)
			texture->Open();

			const TextureRGBA* textureRGBA = (TextureRGBA*)texture->Data_get();

			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, textureRGBA->Sx_get(), textureRGBA->Sy_get(), 0, GL_RGBA, GL_UNSIGNED_BYTE, textureRGBA->Bytes_get());
			GL_CHECKERROR();

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			GL_CHECKERROR();
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			GL_CHECKERROR();

			texture->Close();
#else
	#error
#endif
		}
		else if (texture->IsType("tex_rgba"))
		{
			m_Log.WriteLine(LogLevel_Info, "Uploading RGBA texture");
			
			texture->Open();

			const TextureRGBA* textureRGBA = (TextureRGBA*)texture->Data_get();

#if USE_RGBA8_TEX
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, textureRGBA->Sx_get(), textureRGBA->Sy_get(), 0, GL_RGBA, GL_UNSIGNED_BYTE, textureRGBA->Bytes_get());
			GL_CHECKERROR();
#else
			const uint16_t* bytes = To16BPP(textureRGBA);
			
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, textureRGBA->Sx_get(), textureRGBA->Sy_get(), 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, bytes);
			GL_CHECKERROR();
			
			delete[] bytes;
#endif
			
#if USE_MIPMAPS
			glGenerateMipmapOES(GL_TEXTURE_2D);
			GL_CHECKERROR();
			
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			GL_CHECKERROR();
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			GL_CHECKERROR();
#else
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			GL_CHECKERROR();
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			GL_CHECKERROR();
#endif

			texture->Close();
		}
		else
		{
			throw ExceptionVA("unknown texture type");
		}
		
		texture->DeviceData_set(textureId);
		texture->OnMemoryWarning_set(CallBack(0, HandleMemoryWarning));
		
		m_Log.WriteLine(LogLevel_Info, "Creating texture [done]");
	}
	UsingEnd()
}

void OpenGLUtil::DestroyTexture(IRes* texture)
{
	m_Log.WriteLine(LogLevel_Info, "Destroying texture");
	
	GLuint* textureId = (GLuint*)texture->DeviceData_get();
	
	if (textureId)
	{
		glDeleteTextures(1, textureId);
	
		delete textureId;
	}
	
	texture->DeviceData_set(0);
	texture->OnMemoryWarning_set(CallBack());
	
	m_Log.WriteLine(LogLevel_Info, "Destroying texture [done]");
}

void OpenGLUtil::HandleMemoryWarning(void* obj, void* arg)
{
	m_Log.WriteLine(LogLevel_Info, "Handling memory warning");
	
	OpenGLUtil* self = (OpenGLUtil*)obj;
	
	Res* res = (Res*)arg;
	
	if (res->m_Type == ResTypes_TextureRGBA || res->m_Type == ResTypes_TexturePVR)
		self->DestroyTexture(res);
	
	m_Log.WriteLine(LogLevel_Info, "Handling memory warning [done]");
}

#ifdef IPHONEOS
static void UploadPvr_Iphone(const TexturePVR* texturePVR)
{
	for (size_t i = 0; i < texturePVR->Levels_get().size(); ++i)
	{
		const TexturePVRLevel* level = texturePVR->Levels_get()[i];
		
		glCompressedTexImage2D(GL_TEXTURE_2D, i, GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG, level->m_Sx, level->m_Sy, 0, level->m_DataSize, level->m_Data);
		GL_CHECKERROR();
	}
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	GL_CHECKERROR();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	GL_CHECKERROR();
}
#endif

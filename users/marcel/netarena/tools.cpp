#include "framework.h"
#include "host.h"
#include "main.h"
#include "Options.h"
#include "Path.h"
#include "tools.h"
#include <string>

// spriter animation tests

static bool s_animationTestIsActive = false;
static SpriterState s_animationTestState;
static Spriter * s_animationTestSprite = 0;

void animationTestInit()
{
	auto files = listFiles("testAnimations", true);

	for (auto file : files)
	{
		if (file.find(".scml") == std::string::npos || file.find("autosave") != std::string::npos)
			continue;

		std::string * path = new std::string(file);
		std::string * optionPath = new std::string("Artist Tools/Animation Test/Load '" + Path::GetBaseName(file) + "'");

		g_optionManager.AddCommandOption(optionPath->c_str(),
			[](void * param)
			{
				std::string * path = (std::string*)param;
				s_animationTestState = SpriterState();
				delete s_animationTestSprite;
				s_animationTestSprite = new Spriter(path->c_str());

				if (!animationTestIsActive())
					animationTestToggleIsActive();
			},
			path);
	}
}

bool animationTestIsActive()
{
	return s_animationTestIsActive;
}

void animationTestToggleIsActive()
{
	s_animationTestIsActive = !s_animationTestIsActive;
}

void animationTestChangeAnim(int direction, int x, int y)
{
	if (s_animationTestIsActive && s_animationTestSprite)
	{
		const int animCount = s_animationTestSprite->getAnimCount();

		if (animCount != 0)
		{
			int index = 0;
			if (s_animationTestState.animIndex >= 0)
				index = (s_animationTestState.animIndex + direction + animCount) % animCount;
			s_animationTestState.startAnim(*s_animationTestSprite, index);
			s_animationTestState.x = x;
			s_animationTestState.y = y;
		}
		else
		{
			s_animationTestState.stopAnim(*s_animationTestSprite);
		}
	}
}

void animationTestTick(float dt)
{
	if (s_animationTestIsActive)
	{
		if (mouse.wentDown(BUTTON_LEFT))
			animationTestChangeAnim(0, mouse.x, mouse.y);
		if (mouse.wentDown(BUTTON_RIGHT))
			animationTestChangeAnim(keyboard.isDown(SDLK_LSHIFT) ? -1 : +1, mouse.x, mouse.y);

		if (s_animationTestSprite)
			s_animationTestState.updateAnim(*s_animationTestSprite, dt);
	}
}

void animationTestDraw()
{
	if (s_animationTestIsActive)
	{
		setColor(colorGreen);
		drawRect(0, 0, GFX_SX, 40);

		setColor(colorBlack);
		setFont("calibri.ttf");
		drawText(GFX_SX/2, 20, 24, 0.f, 0.f, "Animation Test (Toggle Using '1')");

		setColor(colorWhite);
		if (s_animationTestSprite)
			s_animationTestSprite->draw(s_animationTestState);
	}
}

// blast effects test

bool s_blastEffectTestIsActive = false;

bool blastEffectTestIsActive()
{
	return s_blastEffectTestIsActive;
}

void blastEffectTestToggleIsActive()
{
	s_blastEffectTestIsActive = !s_blastEffectTestIsActive;
}

void blastEffectTestTick(float dt)
{
	if (s_blastEffectTestIsActive && g_host)
	{
		if (mouse.wentDown(BUTTON_LEFT))
		{
			char temp[256];
			sprintf_s(temp, sizeof(temp), "x:%f y:%f radius:%f speed:%f",
				(float)mouse.x,
				(float)mouse.y,
				500.f,
				500.f);
			g_app->netDebugAction("addBlastEffect", temp);
		}
	}
}

void blastEffectTestDraw()
{
	if (s_blastEffectTestIsActive)
	{
		setColor(colorGreen);
		drawRect(0, 0, GFX_SX, 40);

		setColor(colorBlack);
		setFont("calibri.ttf");
		drawText(GFX_SX/2, 20, 24, 0.f, 0.f, "Blast Effect Test (Toggle Using '2')");

		setColor(colorWhite);
	}
}

// GIF capture tool

#include <FreeImage.h>

static FIBITMAP * dither(FIBITMAP * src, const RGBQUAD * __restrict palette);
static FIBITMAP * ditherQuantize(FIBITMAP * src);
static FIBITMAP * captureScreen();
static FIBITMAP * downsample2x2(FIBITMAP * src, bool freeSrc);
static bool writeGif(const char * filename, const std::vector<FIBITMAP*> & pages, int frameTimeMS);

// todo : arbitrary rectangle selection

static FIBITMAP * dither(FIBITMAP * src, const RGBQUAD * __restrict palette)
{
	const int sx = FreeImage_GetWidth(src);
	const int sy = FreeImage_GetHeight(src);

	FIBITMAP * dst = FreeImage_AllocateEx(sx, sy, 8, &palette[0], 0, palette);

	int * __restrict errorValues = new int[sx * sy * 3];
	memset(errorValues, 0, sx * sy * 3 * sizeof(int));

	for (int y = 0; y < sy; ++y)
	{
		const uint8_t * __restrict srcLine = (uint8_t*)FreeImage_GetScanLine(src, y);
		      uint8_t * __restrict dstLine = (uint8_t*)FreeImage_GetScanLine(dst, y);
		
		for (int x = 0; x < sx; ++x)
		{
			// apply floyd steinberg error diffusion

			const int rDesired = srcLine[2] + errorValues[(x + y * sx) * 3 + 0] / 16;
			const int gDesired = srcLine[1] + errorValues[(x + y * sx) * 3 + 1] / 16;
			const int bDesired = srcLine[0] + errorValues[(x + y * sx) * 3 + 2] / 16;

			srcLine += 3;

			// find the nearest palette entry. this is the slow part

			int bestIndex = -1;
			int bestDistanceSquared = 1 << 30;
			const RGBQUAD * bestQuad = &palette[0];

			for (int i = 0; i < 256; ++i)
			{
				const int rDelta = rDesired - palette[i].rgbRed;
				const int gDelta = gDesired - palette[i].rgbGreen;
				const int bDelta = bDesired - palette[i].rgbBlue;
				const int distanceSquared = rDelta * rDelta + gDelta * gDelta + bDelta * bDelta;

				if (distanceSquared < bestDistanceSquared)
				{
					bestIndex = i;
					bestDistanceSquared = distanceSquared;
					bestQuad = &palette[i];
				}
			}

			// set the palette index

			dstLine[x] = bestIndex;

			// calculate the error and propagate it to the neighboring pixels

			const int rActual = bestQuad->rgbRed;
			const int gActual = bestQuad->rgbGreen;
			const int bActual = bestQuad->rgbBlue;

			const int currentError[3] =
			{
				rDesired - rActual,
				gDesired - gActual,
				bDesired - bActual
			};

			for (int i = 0; i < 3; ++i)
			{
				if (x + 1 < sx && y + 0 < sy) errorValues[((x + 1) + (y + 0) * sx) * 3 + i] += currentError[i] * 7;
				if (x + 0 < sx && y + 1 < sy) errorValues[((x + 0) + (y + 1) * sx) * 3 + i] += currentError[i] * 3;
				if (x + 1 < sx && y + 1 < sy) errorValues[((x + 1) + (y + 1) * sx) * 3 + i] += currentError[i] * 5;
				if (x + 2 < sx && y + 1 < sy) errorValues[((x + 2) + (y + 1) * sx) * 3 + i] += currentError[i] * 1;
			}
		}
	}

	delete [] errorValues;

	return dst;
}

static FIBITMAP * ditherQuantize(FIBITMAP * src)
{
	// quantize the original bitmap, so we know which palette to use
	FIBITMAP * quantized = FreeImage_ColorQuantize(src, FIQ_WUQUANT);
	const RGBQUAD * palette = FreeImage_GetPalette(quantized);
	// dither the original image using the palette we just got
	FIBITMAP * result = dither(src, palette);
	FreeImage_Unload(quantized);
	return result;
}

static FIBITMAP * captureScreen()
{
	const int sx = GFX_SX / framework.minification;
	const int sy = GFX_SY / framework.minification;
	uint8_t * __restrict pixels = new uint8_t[sx * sy * 4];
	glReadPixels(0, 0, sx, sy, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

	FIBITMAP * result = FreeImage_Allocate(sx, sy, 24);

	for (int y = 0; y < sy; ++y)
	{
		const uint8_t * __restrict srcLine = &pixels[y * sx * 4];
		      uint8_t * __restrict dstLine = FreeImage_GetScanLine(result, y);
		
		for (int x = 0; x < sx; ++x)
		{
			dstLine[2] = srcLine[0];
			dstLine[1] = srcLine[1];
			dstLine[0] = srcLine[2];

			srcLine += 4;
			dstLine += 3;
		}
	}

	delete [] pixels;

	return result;
}

static FIBITMAP * downsample2x2(FIBITMAP * src, bool freeSrc)
{
	const int sx = FreeImage_GetWidth(src) / 2;
	const int sy = FreeImage_GetHeight(src) / 2;

	FIBITMAP * dst = FreeImage_Allocate(sx, sy, 24);

	for (int y = 0; y < sy; ++y)
	{
		const uint8_t * __restrict srcLine1 = (uint8_t*)FreeImage_GetScanLine(src, y * 2 + 0);
		const uint8_t * __restrict srcLine2 = (uint8_t*)FreeImage_GetScanLine(src, y * 2 + 1);
		      uint8_t * __restrict dstLine = (uint8_t*)FreeImage_GetScanLine(dst, y);
		
		for (int x = 0; x < sx; ++x)
		{
			for (int i = 0; i < 3; ++i)
				dstLine[i] = (srcLine1[i + 0] + srcLine1[i + 3] + srcLine2[i + 0] + srcLine2[i + 3]) / 4;

			srcLine1 += 3 * 2;
			srcLine2 += 3 * 2;
			dstLine += 3 * 1;
		}
	}

	if (freeSrc)
		FreeImage_Unload(src);

	return dst;
}

static bool writeGif(const char * filename, const std::vector<FIBITMAP*> & pages, int frameTimeMS)
{
	FIMULTIBITMAP * bmp = FreeImage_OpenMultiBitmap(FIF_GIF, filename, TRUE, FALSE, TRUE, GIF_DEFAULT);
	for (size_t i = 0; i < pages.size(); ++i)
	{
		FIBITMAP * page = ditherQuantize(pages[i]);
		FITAG * tag = FreeImage_CreateTag();
		if (tag)
		{
			LONG frameTime = frameTimeMS;
			BOOL result = TRUE;
			result &= FreeImage_SetTagID(tag, 4101);
			result &= FreeImage_SetTagKey(tag, "FrameTime");
			result &= FreeImage_SetTagType(tag, FIDT_LONG);
			result &= FreeImage_SetTagCount(tag, 1);
			result &= FreeImage_SetTagLength(tag, 4);
			result &= FreeImage_SetTagValue(tag, &frameTime);
			result &= FreeImage_SetMetadata(FIMD_ANIMATION, page, FreeImage_GetTagKey(tag), tag);
			FreeImage_DeleteTag(tag);
		}
		FreeImage_AppendPage(bmp, page);
	}
	return (FreeImage_CloseMultiBitmap(bmp) == TRUE);
}

//

OPTION_DECLARE(bool, g_screenCapture, false);
OPTION_DECLARE(int, g_screenCaptureDuration, 5);
OPTION_DECLARE(float, g_screenCaptureDownscaleFactor, 2.f);
OPTION_DECLARE(int, g_screenCaptureFrameSkip, 4);
OPTION_DEFINE(bool, g_screenCapture, "App/GIF Capture/Capture! (Full Screen)");
OPTION_DEFINE(int, g_screenCaptureDuration, "App/GIF Capture/Duration (Seconds)");
OPTION_DEFINE(float, g_screenCaptureDownscaleFactor, "App/GIF Capture/Downscale Factor");
OPTION_DEFINE(int, g_screenCaptureFrameSkip, "App/GIF Capture/Frame Skip");
OPTION_STEP(g_screenCaptureDownscaleFactor, 0.f, 0.f, .1f);

static bool s_gifCaptureIsActive = false;
static int s_skipFrames = 0;
static int s_numFrames = 0;
static std::vector<FIBITMAP*> s_pages;

void gifCaptureToggleIsActive(bool cancel)
{
	if (s_gifCaptureIsActive)
		gifCaptureComplete(cancel);

	s_gifCaptureIsActive = !s_gifCaptureIsActive;

	if (s_gifCaptureIsActive)
		fassert(s_pages.empty());
}

void gifCaptureComplete(bool cancel)
{
	fassert(s_gifCaptureIsActive);

	if (cancel)
	{
		for (auto page : s_pages)
			FreeImage_Unload(page);
		s_pages.clear();
	}
	else
	{
		writeGif("c:/temp/test2.gif", s_pages, 1000 * g_screenCaptureFrameSkip / 60);
	}

	s_skipFrames = 0;
	s_numFrames = 0;
	s_pages.clear();
}

void gifCaptureTick(float dt)
{
	if (!s_gifCaptureIsActive)
		return;
}

void gifCaptureTick_PostRender()
{
	if (!s_gifCaptureIsActive)
		return;

	const bool screenCapture = g_screenCapture;
	if (screenCapture)
	{
		g_screenCapture = false;
		g_app->m_optionMenuIsOpen = false;
	}

	if (screenCapture)
	{
		s_skipFrames = 0;
		s_numFrames = 60 * g_screenCaptureDuration / g_screenCaptureFrameSkip;
	}

	if ((s_skipFrames++ % g_screenCaptureFrameSkip) == 0)
	{
		if (s_numFrames > 0)
		{
			FIBITMAP * capture = captureScreen();
			capture = downsample2x2(capture, true);
			//capture = downsample2x2(capture, true);
			s_pages.push_back(capture);

			s_numFrames--;

			if (s_numFrames == 0)
				gifCaptureComplete(false);
		}
	}
}

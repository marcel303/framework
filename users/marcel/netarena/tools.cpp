#include "FileStream.h"
#include "framework.h"
#include "host.h"
#include "main.h"
#include "Options.h"
#include "Path.h"
#include "tools.h"
#include <string>

#include "Timer.h" // fixme

#if ENABLE_DEVMODE

#define ANIM_COLOR Color(255, 255, 127)
#define BLAST_COLOR Color(255, 127, 127)
#define GIF_COLOR Color(127, 127, 255)

static void drawToolCaption(const Color & backColor, const char * format, ...)
{
	char text[1024];
	va_list args;
	va_start(args, format);
	vsprintf_s(text, sizeof(text), format, args);
	va_end(args);

	setColor(backColor);
	drawRect(0, 0, GFX_SX, 40);

	setColor(colorBlack);
	setDebugFont();
	drawText(GFX_SX/2, 20, 24, 0.f, 0.f, text);
}

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
		drawToolCaption(ANIM_COLOR, "Animation Test. LEFT = Trigger, RIGHT = Next, SHIFT+RIGHT = Previous");

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
		drawToolCaption(BLAST_COLOR, "Blast Effect Test. LEFT = Add Blast");
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

struct kd_node_data_t
{
	int rgb[3];
	int index;
};

struct kd_node_t : kd_node_data_t
{
	kd_node_t * left;
	kd_node_t * right;
};

inline int dist(const kd_node_data_t * a, const kd_node_data_t * b)
{
	const int dr = a->rgb[0] - b->rgb[0];
	const int dg = a->rgb[1] - b->rgb[1];
	const int db = a->rgb[2] - b->rgb[2];
	return dr * dr + dg * dg + db * db;
}

inline void swap(kd_node_data_t * __restrict x, kd_node_data_t * __restrict y)
{
	std::swap(*x, *y);
}

// see quickselect method

kd_node_t * find_median(kd_node_t * start, kd_node_t * end, int idx)
{
	if (end <= start)
		return 0;

	if (end == start + 1)
		return start;

	kd_node_t * md = start + (end - start) / 2;

	for (;;)
	{
		const int pivot = md->rgb[idx];

		swap(md, end - 1);

		kd_node_t * p;
		kd_node_t * store;

		for (store = p = start; p < end; p++)
		{
			if (p->rgb[idx] < pivot)
			{
				if (p != store)
					swap(p, store);

				store++;
			}
		}

		swap(store, end - 1);

		// median has duplicate values

		if (store->rgb[idx] == md->rgb[idx])
			return md;

		if (store > md)
			end = store;
		else
			start = store;
	}
}

kd_node_t * make_tree(kd_node_t * t, int len, int i)
{
	if (!len)
		return 0;

	kd_node_t * n;

	if ((n = find_median(t, t + len, i)))
	{
		i = (i + 1) % 3;
		n->left  = make_tree(t, n - t, i);
		n->right = make_tree(n + 1, t + len - (n + 1), i);
	}

	return n;
}

void nearest(
	kd_node_t * __restrict root,
	kd_node_data_t * __restrict nd,
	int i,
	kd_node_t * __restrict & best,
	int & best_dist)
{
	if (!root)
		return;

	const int d = dist(root, nd);
	const int dx = root->rgb[i] - nd->rgb[i];
	const int dx2 = dx * dx;

	if (!best || d < best_dist)
	{
		best_dist = d;
		best = root;
	}

	// if chance of exact match is high

	if (!best_dist)
		return;

	if (++i == 3)
		i = 0;

	// todo : compute bb min/max?

	nearest(dx > 0 ? root->left : root->right, nd, i, best, best_dist);

	if (dx2 >= best_dist)
		return;

	nearest(dx > 0 ? root->right : root->left, nd, i, best, best_dist);
}

//

#define USE_KD_TREE 1
#define OPTIMIZED_ERROR_DIFFUSION 1

static FIBITMAP * dither(FIBITMAP * src, const RGBQUAD * __restrict palette)
{
#if USE_KD_TREE
	const float t1 = g_TimerRT.Time_get();
	kd_node_t nodes[256];

	for (int i = 0; i < 256; ++i)
	{
		nodes[i].rgb[0] = palette[i].rgbRed;
		nodes[i].rgb[1] = palette[i].rgbGreen;
		nodes[i].rgb[2] = palette[i].rgbBlue;
		nodes[i].index = i;
	}

	kd_node_t * root = make_tree(nodes, 256, 0);
	const float t2 = g_TimerRT.Time_get();
	printf("t took %03.2fms\n", (t2 - t1) * 1000.f);
#endif

	const int sx = FreeImage_GetWidth(src);
	const int sy = FreeImage_GetHeight(src);

	FIBITMAP * dst = FreeImage_AllocateEx(sx, sy, 8, &palette[0], 0, palette);

#if OPTIMIZED_ERROR_DIFFUSION
	int * __restrict errorValues = new int[(sx + 2) * (sy + 1) * 3];
	memset(errorValues, 0, (sx + 2) * (sy + 1) * 3 * sizeof(int));
#else
	int * __restrict errorValues = new int[sx * sy * 3];
	memset(errorValues, 0, sx * sy * 3 * sizeof(int));
#endif

	for (int y = 0; y < sy; ++y)
	{
		const uint8_t * __restrict srcLine = (uint8_t*)FreeImage_GetScanLine(src, y);
		      uint8_t * __restrict dstLine = (uint8_t*)FreeImage_GetScanLine(dst, y);
		
	#if OPTIMIZED_ERROR_DIFFUSION
		int * __restrict errorLine1 = &errorValues[(y + 0) * (sx + 2) * 3];
		int * __restrict errorLine2 = &errorValues[(y + 1) * (sx + 2) * 3];
	#endif

		for (int x = 0; x < sx; ++x)
		{
			// apply floyd steinberg error diffusion

		#if OPTIMIZED_ERROR_DIFFUSION
			const int rDesired = srcLine[2] + errorLine1[0] / 16;
			const int gDesired = srcLine[1] + errorLine1[1] / 16;
			const int bDesired = srcLine[0] + errorLine1[2] / 16;
		#else
			const int rDesired = srcLine[2] + errorValues[(x + y * sx) * 3 + 0] / 16;
			const int gDesired = srcLine[1] + errorValues[(x + y * sx) * 3 + 1] / 16;
			const int bDesired = srcLine[0] + errorValues[(x + y * sx) * 3 + 2] / 16;
		#endif

			srcLine += 3;

		#if USE_KD_TREE
			kd_node_data_t lookup;
			lookup.rgb[0] = rDesired;
			lookup.rgb[1] = gDesired;
			lookup.rgb[2] = bDesired;

			kd_node_t * best = 0;
			int bestDist;
			nearest(root, &lookup, 0, best, bestDist);

			const int bestIndex = best->index;
			const RGBQUAD * bestQuad = &palette[bestIndex];
		#else
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
		#endif

			// set the palette index

			dstLine[x] = bestIndex;

		#if 1
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
			#if OPTIMIZED_ERROR_DIFFUSION
				errorLine1[3 + i] += currentError[i] * 7;
				errorLine2[0 + i] += currentError[i] * 3;
				errorLine2[3 + i] += currentError[i] * 5;
				errorLine2[6 + i] += currentError[i] * 1;
			#else
				if (x + 1 < sx && y + 0 < sy) errorValues[((x + 1) + (y + 0) * sx) * 3 + i] += currentError[i] * 7;
				if (x + 0 < sx && y + 1 < sy) errorValues[((x + 0) + (y + 1) * sx) * 3 + i] += currentError[i] * 3;
				if (x + 1 < sx && y + 1 < sy) errorValues[((x + 1) + (y + 1) * sx) * 3 + i] += currentError[i] * 5;
				if (x + 2 < sx && y + 1 < sy) errorValues[((x + 2) + (y + 1) * sx) * 3 + i] += currentError[i] * 1;
			#endif
			}
		#endif

		#if OPTIMIZED_ERROR_DIFFUSION
			errorLine1 += 3;
			errorLine2 += 3;
		#endif
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
	const float t1 = g_TimerRT.Time_get();
	FIBITMAP * result = dither(src, palette);
	const float t2 = g_TimerRT.Time_get();
	printf("q took %03.2fms\n", (t2 - t1) * 1000.f);
	FreeImage_Unload(quantized);
	return result;
}

static FIBITMAP * captureScreen(int ox, int oy, int sx, int sy)
{
	uint8_t * __restrict pixels = new uint8_t[sx * sy * 4];

	pushSurface(g_finalMap);
	{
		glReadPixels(ox, oy, sx, sy, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
	}
	popSurface();

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
	if (frameTimeMS < 20)
		frameTimeMS = 20;

	FIMULTIBITMAP * bmp = FreeImage_OpenMultiBitmap(FIF_GIF, filename, TRUE, FALSE, TRUE, GIF_DEFAULT);
	for (size_t i = 0; i < pages.size(); ++i)
	{
		const float t1 = g_TimerRT.Time_get();
		FIBITMAP * page = ditherQuantize(pages[i]);
		const float t2 = g_TimerRT.Time_get();
		printf("dq took %03.2fms\n", (t2 - t1) * 1000.f);

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
OPTION_DECLARE(int, g_screenCaptureDuration, 4);
OPTION_DECLARE(int, g_screenCaptureDownscalePasses, 1);
OPTION_DECLARE(int, g_screenCaptureFrameSkip, 1);
OPTION_DEFINE(bool, g_screenCapture, "App/GIF Capture/Capture! (Full Screen)");
OPTION_DEFINE(int, g_screenCaptureDuration, "App/GIF Capture/Duration (Seconds)");
OPTION_DEFINE(int, g_screenCaptureDownscalePasses, "App/GIF Capture/Number of Downscale Passes");
OPTION_DEFINE(int, g_screenCaptureFrameSkip, "App/GIF Capture/Frame Skip");
OPTION_STEP(g_screenCaptureDownscalePasses, 0, 4, 1);
OPTION_STEP(g_screenCaptureFrameSkip, 1, 60, 1);

enum GifCaptureState
{
	kGifCapture_Idle,
	kGifCapture_AreaSelect1,
	kGifCapture_AreaSelect2,
	kGifCapture_PressHotkey,
	kGifCapture_Capture,
};

static GifCaptureState s_gifCaptureState = kGifCapture_Idle;
struct
{
	float x1, y1, x2, y2;
} s_gifCaptureRect;
static int s_skipFrames = 0;
static int s_numFrames = 0;
static int s_numCapturedFrames = 0;
static std::vector<FIBITMAP*> s_pages;

extern Surface * g_finalMap;

void gifCaptureToggleIsActive(bool cancel)
{
	if (s_gifCaptureState == kGifCapture_Idle)
		s_gifCaptureState = kGifCapture_AreaSelect1;
	else if (s_gifCaptureState == kGifCapture_Capture)
		gifCaptureEnd(cancel);
	else
		s_gifCaptureState = kGifCapture_Idle;

	if (s_gifCaptureState == kGifCapture_Idle)
		fassert(s_pages.empty());
}

void gifCaptureBegin()
{
	s_gifCaptureState = kGifCapture_Capture;

	s_skipFrames = 0;
	s_numFrames = 60 * g_screenCaptureDuration / g_screenCaptureFrameSkip;
	s_numCapturedFrames = 0;

	g_screenCapture = false;
	g_app->m_optionMenuIsOpen = false;
}

void gifCaptureEnd(bool cancel)
{
	fassert(s_gifCaptureState == kGifCapture_Capture);

	if (cancel)
	{
		s_gifCaptureState = kGifCapture_Idle;
	}
	else
	{
		s_gifCaptureState = kGifCapture_PressHotkey;

		char filename[256];
		std::string userFolder = g_app->getUserSettingsDirectory();
		int i = 0;
		do
		{
			sprintf_s(filename, sizeof(filename), "%s/MovieCapture%03d.gif", userFolder.c_str(), i);
			i++;
		} while (FileStream::Exists(filename));
		writeGif(filename, s_pages, 1000 * g_screenCaptureFrameSkip / 60);
	}

	for (auto page : s_pages)
		FreeImage_Unload(page);
	s_pages.clear();
}

void gifCaptureTick(float dt)
{
	switch (s_gifCaptureState)
	{
	case kGifCapture_Idle:
		break;
	case kGifCapture_AreaSelect1:
		if (mouse.wentDown(BUTTON_LEFT))
		{
			s_gifCaptureRect.x1 = s_gifCaptureRect.x2 = mouse.x;
			s_gifCaptureRect.y1 = s_gifCaptureRect.y2 = mouse.y;
			s_gifCaptureState = kGifCapture_AreaSelect2;
		}
		break;
	case kGifCapture_AreaSelect2:
		s_gifCaptureRect.x2 = mouse.x;
		s_gifCaptureRect.y2 = mouse.y;
		if (mouse.wentUp(BUTTON_LEFT))
			s_gifCaptureState = kGifCapture_PressHotkey;
		break;
	case kGifCapture_PressHotkey:
		if (gamepad[0].wentDown(GAMEPAD_BACK))
			gifCaptureBegin();
		break;
	case kGifCapture_Capture:
		break;
	default:
		fassert(false);
	}
}

void gifCaptureTick_PostRender()
{
	switch (s_gifCaptureState)
	{
	case kGifCapture_Idle:
		break;
	case kGifCapture_AreaSelect1:
		drawToolCaption(GIF_COLOR, "GIF Capture : Select Rectangle");
		break;
	case kGifCapture_AreaSelect2:
		drawToolCaption(GIF_COLOR, "GIF Capture : Select Rectangle");

		setColor(colorGreen);
		drawRectLine(
			s_gifCaptureRect.x1,
			s_gifCaptureRect.y1,
			s_gifCaptureRect.x2,
			s_gifCaptureRect.y2);
		break;
	case kGifCapture_PressHotkey:
		drawToolCaption(GIF_COLOR, "GIF Capture : Press BACK To Start Recording");

		setColor(colorGreen);
		drawRectLine(
			s_gifCaptureRect.x1,
			s_gifCaptureRect.y1,
			s_gifCaptureRect.x2,
			s_gifCaptureRect.y2);
		break;
	case kGifCapture_Capture:
		fassert(s_numCapturedFrames < s_numFrames);

		if ((s_skipFrames++ % g_screenCaptureFrameSkip) == 0)
		{
			if (s_gifCaptureRect.x1 > s_gifCaptureRect.x2)
				std::swap(s_gifCaptureRect.x1, s_gifCaptureRect.x2);
			if (s_gifCaptureRect.y1 > s_gifCaptureRect.y2)
				std::swap(s_gifCaptureRect.y1, s_gifCaptureRect.y2);

			const int ox = s_gifCaptureRect.x1;
			const int oy = (GFX_SY - s_gifCaptureRect.y2);
			const int sx = (s_gifCaptureRect.x2 - s_gifCaptureRect.x1);
			const int sy = (s_gifCaptureRect.y2 - s_gifCaptureRect.y1);

			FIBITMAP * capture = captureScreen(ox, oy, sx, sy);
			for (int i = 0; i < g_screenCaptureDownscalePasses; ++i)
				capture = downsample2x2(capture, true);
			s_pages.push_back(capture);

			s_numCapturedFrames++;

			if (s_numCapturedFrames == s_numFrames)
				gifCaptureEnd(false);
		}

		setColor(255, 255, 255, 127);
		drawRectLine(
			s_gifCaptureRect.x1,
			s_gifCaptureRect.y1,
			s_gifCaptureRect.x2,
			s_gifCaptureRect.y2);

		drawToolCaption(GIF_COLOR, "GIF Capture : Capturing Frame %d/%d", s_numCapturedFrames, s_numFrames);
		break;
	default:
		fassert(false);
	}
}

#endif

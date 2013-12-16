#include <deque>
#include <SDL/SDL.h>
#include "Application.h"
#include "Arguments.h"
#include "Calc.h"
#include "CallBack.h"
#include "FileArchive.h"
#include "FileStream.h"
#include "Image.h"
#include "ImageLoader_FreeImage.h"
#include "ImageLoader_Photoshop.h"
#include "KlodderSystem.h"
#include "LayerMgr.h"
#include "Parse.h"
#include "Path.h"
#if USE_QUICKTIME
#include "QuickTimeEncoder.h"
#endif
#include "Settings.h"
#include "StreamProvider_Replay.h"
#include "StreamReader.h"
#include "StringEx.h"
#include "Timer.h"
#include "Util_Mem.h"

#include "KlodderTestCode.h"
#include "MouseMgr.h"

#define FRAME_SKIP 5

#define DEFAULT_LAYERCOUNT 3
#define DEFAULT_SX 320
#define DEFAULT_SY 480

static void HandleTouchBegin(void* obj, void* arg);
static void HandleTouchEnd(void* obj, void* arg);
static void HandleTouchMove(void* obj, void* arg);

enum RequestType
{
	RequestType_Paint,
	RequestType_RenderToMovie,
	RequestType_RenderToPicture,
	RequestType_Replay,
};

class AppSettings
{
public:
	AppSettings()
	{
		requestType = RequestType_Replay;
		scale = 1;
		backColor1 = Rgba_Make(0.9f, 0.9f, 0.9f, 1.0f);
		backColor2 = Rgba_Make(0.8f, 0.8f, 0.8f, 1.0f);
		preview = false;
		movieFps = 30;
		movieTrail = 0;
		replayFps = 0;
		showProgress = true;
		waitKey = false;
	}

	void Validate()
	{
		switch (requestType)
		{
		case RequestType_RenderToMovie:
			if (src == String::Empty)
				throw ExceptionVA("source not set");
			if (dst == String::Empty)
				throw ExceptionVA("destination not set");
			if (scale < 1 || scale > 4)
				throw ExceptionVA("invalid scale: %d", scale);
			if (movieFps < 1)
				throw ExceptionVA("invalid movie FPS: %d", movieFps);
			if (movieTrail < 0)
				throw ExceptionVA("invalid movie trail: %d", movieTrail);
			break;
		case RequestType_RenderToPicture:
			if (src == String::Empty)
				throw ExceptionVA("source not set");
			if (dst == String::Empty)
				throw ExceptionVA("destination not set");
			if (scale < 1 || scale > 4)
				throw ExceptionVA("invalid scale: %d", scale);
			break;
		case RequestType_Replay:
			if (src == String::Empty)
				throw ExceptionVA("source not set");
			if (scale < 1 || scale > 4)
				throw ExceptionVA("invalid scale: %d", scale);
			if (replayFps < 0)
				throw ExceptionVA("invalid replay FPS: %d", movieFps);
			break;
		case RequestType_Paint:
			break;
		}
	}

	RequestType requestType;
	std::string src;
	std::string dst;
	int scale;
	Rgba backColor1;
	Rgba backColor2;
	bool preview;
	int movieFps;
	int movieTrail;
	int replayFps;
	bool showProgress;
	bool waitKey;
};

static AppSettings sSettings;

class SDL_SurfacePP
{
public:
	SDL_SurfacePP(SDL_Surface* surface)
	{
		mSurface = surface;

		shift[0] = mSurface->format->Rshift;
		shift[1] = mSurface->format->Gshift;
		shift[2] = mSurface->format->Bshift;
	}

	inline uint32_t MakeCol(int r, int g, int b) const
	{
		return
			r << shift[0] |
			g << shift[1] |
			b << shift[2];
	}

	inline int Sx_get() const
	{
		return mSurface->w;
	}

	inline int Sy_get() const
	{
		return mSurface->h;
	}

	AreaI Area_get() const
	{
		return AreaI(Vec2I(0, 0), Vec2I(Sx_get() - 1, Sy_get() - 1));
	}

	uint32_t* Line_get(int y)
	{
		uint32_t* pixels = (uint32_t*)mSurface->pixels;

		return pixels + y * mSurface->w;
	}

	void DrawRect(int x, int y, int sx, int sy, uint32_t c)
	{
		AreaI area(Vec2I(x, y), Vec2I(x + sx - 1, y + sy - 1));

		if (!area.Clip(Area_get()))
			return;

		for (int y = area.y1; y <= area.y2; ++y)
		{
			uint32_t* line = Line_get(y);

			for (int x = area.x1; x <= area.x2; ++x)
				line[x] = c;
		}
	}

	void DrawLineH(int x, int y, int sx, uint32_t c)
	{
		DrawRect(x, y, sx, 1, c);
	}

	void DrawLineV(int x, int y, int sy, uint32_t c)
	{
		DrawRect(x, y, 1, sy, c);
	}

	void DrawBevel(int x, int y, int sx, int sy, uint32_t c1, uint32_t c2)
	{
		DrawLineV(x + sx - 1, y, sy, c2);
		DrawLineH(x, y + sy - 1, sx, c2);
		DrawLineH(x, y, sx, c1);
		DrawLineV(x, y, sy, c1);
	}

private:
	SDL_Surface* mSurface;
	int shift[3];
};

static void CreateApplication(Application& application, ImageId imageId, int layerCount, int sx, int sy, bool writeEnabled, bool undoEnabled, FileArchive* archive)
{
	LOG_INF("application: setup", 0);

	if (!imageId.IsSet_get())
	{
		if (writeEnabled)
			throw ExceptionVA("image id not set");
		imageId = ImageId("999");
	}

	application.Setup(
		archive ? new StreamProvider_Replay(archive->GetStream("data")) : 0,
		gSystem.GetResourcePath("brushes_hq.lib").c_str(),
		gSystem.GetResourcePath("brushes_hq.lib").c_str(),
		sSettings.scale,
		sSettings.backColor1,
		sSettings.backColor2);

	application.ImageReset(imageId);
	application.ImageActivate(writeEnabled);
	application.ImageInitialize(layerCount, sx, sy);
	application.LayerMgr_get()->EditingBegin(true);

	application.UndoEnabled_set(undoEnabled);

	LOG_INF("application: setup [done]", 0);
}

static void RenderProgressBar(SDL_SurfacePP& surface, int x, int y, int sx, int sy, int count, int progress)
{
	float t = 1.0f;
	
	if (count > 0)
		t = progress / (float)count;

	surface.DrawRect(x, y, sx, sy, surface.MakeCol(0, 0, 0));
	const int border = 2;
	sx = (int)((sx - border * 2) * t);
	sy = (sy - border * 2);
	surface.DrawRect(x + border, y + border, sx, sy, surface.MakeCol(127, 140, 127));
	surface.DrawBevel(x + border, y + border, sx, sy, surface.MakeCol(255, 255, 255), surface.MakeCol(63, 63, 63));
}

class PlaybackState
{
public:
	PlaybackState()
	{
		tag = 0;
		application = 0;
		screen = 0;
		packetCount = 0;
		packetIndex = 0;
		frameMS = 0;
		showProgress = false;
	}

	void* tag;
	Application* application;
	SDL_Surface* screen;
	AreaI area;
	int packetCount;
	int packetIndex;
	int frameMS;
	bool showProgress;
};

typedef void (*PlaybackCB)(PlaybackState* state);

static void Preview(AreaI area, Application* application, SDL_Surface* surface, bool preview, int packetCount, int packetIndex, bool showProgress)
{
	Assert(area.IsSet_get());
	
	if (SDL_LockSurface(surface) == 0)
	{
		SDL_SurfacePP surfacePP(surface);

		if (preview)
		{
			const MacImage* image2 = application->LayerMgr_get()->Merged_get();

			uint32_t* pixels = (uint32_t*)surface->pixels;

			const int shiftR = surface->format->Rshift;
			const int shiftG = surface->format->Gshift;
			const int shiftB = surface->format->Bshift;

			for (int y = area.y1; y <= area.y2; ++y)
			{
				const MacRgba* srcLine = image2->Line_get(y) + area.x1;
				uint32_t* dstLine = pixels + y * surface->w + area.x1;

				for (int x = area.x1; x <= area.x2; ++x)
				{
					const int r = srcLine->rgba[0];
					const int g = srcLine->rgba[1];
					const int b = srcLine->rgba[2];

					*dstLine =
						r << shiftR |
						g << shiftG |
						b << shiftB;

					srcLine++;
					dstLine++;
				}
			}
		}

		if (showProgress)
		{
			const int margin = 5;

			RenderProgressBar(surfacePP, margin, margin, surfacePP.Sx_get() - margin * 2, 20, packetCount, packetIndex);
		}

		SDL_UnlockSurface(surface);
	}
}

static void Playback(Application* application, FileArchive* archive, bool preview, int frameMS, bool showProgress, PlaybackCB playbackCb, void* tag)
{
	PlaybackState state;
	state.tag = tag;
	state.application = application;
	state.screen = 0;
	state.frameMS = frameMS;
	state.showProgress = showProgress;

	std::deque<CommandPacket> packetList;

	// load recording

	LOG_INF("reading recording", 0);

	Stream* streamRec = archive->GetStream("recording");

	StreamReader reader(streamRec, false);

	while (!streamRec->EOF_get())
	{
		CommandPacket packet;

		packet.Read(reader);

		packetList.push_back(packet);
	}

	LOG_INF("read %lu frames", packetList.size());

	// load brushes

	Stream* streamBrushes = archive->GetStream("brushes");

	BrushLibrary* library = new BrushLibrary();

	library->Load(streamBrushes, true);

	application->BrushLibrarySet_get()->Add(library, false);

	// disable undo

	application->UndoEnabled_set(false);

	//

	state.packetCount = packetList.size();
	state.packetIndex = 0;

	bool stop = false;

	Timer timer;

	float prevTime = timer.Time_get();

	while (!stop)
	{
		SDL_Event e;

		while (SDL_PollEvent(&e))
		{
			if (e.type == SDL_KEYDOWN)
			{
				if (e.key.keysym.sym == SDLK_ESCAPE)
				{
					stop = true;
				}
			}
		}

		if (packetList.size() > 0)
		{
			CommandPacket packet = packetList.front();
			packetList.pop_front();
			
			application->Execute(packet);

			if (true)
			{
				const Vec2I size = preview ? application->LayerMgr_get()->Size_get() : Vec2I(320, 30);

				if (size[0] * size[1] > 0 && (!state.screen || size[0] != state.screen->w || size[1] != state.screen->h))
				{
					if (state.screen)
					{
						SDL_FreeSurface(state.screen);
						state.screen = 0;
					}

					const int screenFlags = SDL_SWSURFACE;

					state.screen = SDL_SetVideoMode(size[0], size[1], 32, screenFlags);
				}
			}

			state.packetIndex++;

			const AreaI area = application->LayerMgr_get()->Validate();

			if (area.IsSet_get())
			{
				const float currTime = timer.Time_get();
				const int deltaMS = (int)((currTime - prevTime) * 1000.0f);
				const int waitMS = state.frameMS - deltaMS;
				if (waitMS > 0)
					SDL_Delay(waitMS);
				prevTime = currTime;

				state.area = area;
				playbackCb(&state);
			}
		}
		else
		{
			stop = true;
		}
	}
}

#if USE_QUICKTIME

class RenderMovieState
{
public:
	RenderMovieState()
	{
		preview = false;
		encoder = 0;
		frameDuration = 0;
		frameCount = 0;
		movieTrail = 0;
	}

	bool preview;
	std::string dst;
	QuickTimeEncoder* encoder;
	MacImage image;
	int frameDuration;
	int frameCount;
	int movieTrail;
};

static void RenderMovieCb(PlaybackState* state);

static void RenderMovie(FileArchive* archive, std::string dst, int fps, bool preview, bool showProgress, int movieTrail)
{
	Application application;

	CreateApplication(application, ImageId(), 3, 320, 480, false, false, archive);

	QuickTimeEncoder encoder;

	RenderMovieState state;
	state.preview = preview;
	state.dst = dst;
	state.encoder = &encoder;
	state.frameDuration = 1000 / fps;
	state.frameCount = 0;
	state.movieTrail = movieTrail;

	Playback(&application, archive, state.preview, 0, showProgress, RenderMovieCb, &state);

	for (int i = 0; i < state.movieTrail; ++i)
		encoder.CommitVideoFrame(state.frameDuration);

	if (encoder.IsInitialized_get())
		encoder.Shutdown();

	application.ImageDeactivate();
}

static void RenderMovieCb(PlaybackState* state)
{
	RenderMovieState* renderState = (RenderMovieState*)state->tag;

	if (!renderState->encoder->IsInitialized_get())
	{
		const Vec2I size = state->application->LayerMgr_get()->Size_get();

		renderState->image.Size_set(size[0], size[1], false);
			
		renderState->encoder->Initialize(
			renderState->dst.c_str(),
			QtVideoCodec_H264,
			QtVideoQuality_High,
			&renderState->image);
	}

	if (state->area.IsSet_get() && renderState->encoder->IsInitialized_get())
	{
		LOG_INF("committing frame %d..", renderState->frameCount);

		const MacImage* src = state->application->LayerMgr_get()->Merged_get();

		for (int y = 0; y < renderState->image.Sy_get(); ++y)
		{
			const MacRgba* srcLine = src->Line_get(y);
			MacRgba* dstLine = renderState->image.Line_get(renderState->image.Sy_get() - 1 - y);

			Mem::Copy(srcLine, dstLine, sizeof(MacRgba) * renderState->image.Sx_get());
		}

		renderState->encoder->CommitVideoFrame(renderState->frameDuration);

		renderState->frameCount++;

		//

		if (true)
		{
			if (renderState->preview || (state->packetIndex % FRAME_SKIP == 0))
			{
				Preview(state->area, state->application, state->screen, renderState->preview, state->packetCount, state->packetIndex, state->showProgress);

				SDL_Flip(state->screen);
			}
		}
	}
}

#endif

class RenderPictureState
{
public:
	bool preview;
	std::string dst;
};

static void RenderPictureCb(PlaybackState* state);

static void MacImageToImage(const MacImage& src, Image& dst)
{
	dst.SetSize(src.Sx_get(), src.Sy_get());

	for (int y = 0; y < src.Sy_get(); ++y)
	{
		const MacRgba* srcLine = src.Line_get(y);
		ImagePixel* dstLine = dst.GetLine(y);

		for (int x = 0; x < src.Sx_get(); ++x)
		{
			dstLine[x].r = srcLine[x].rgba[0];
			dstLine[x].g = srcLine[x].rgba[1];
			dstLine[x].b = srcLine[x].rgba[2];
			dstLine[x].a = srcLine[x].rgba[3];
		}
	}
}

static void RenderPicture(FileArchive* archive, std::string dst, bool preview, bool showProgress)
{
	Application application;
	
	CreateApplication(application, ImageId(), 3, 320, 480, false, false, archive);

	RenderPictureState state;
	state.preview = preview;
	state.dst = dst;

	Playback(&application, archive, preview, 0, showProgress, RenderPictureCb, &state);

	MacImage merged;
	application.LayerMgr_get()->RenderMergedFinal(merged);

	if (!String::EndsWith(dst, ".psd"))
	{
		ImageLoader_FreeImage loader;
		Image image;
		MacImageToImage(merged, image);
		loader.Save(image, state.dst);
	}
	else
	{
		ImageLoader_Photoshop loader;
		Image** imageList = new Image*[application.LayerMgr_get()->LayerCount_get()];
		for (int i = 0; i < application.LayerMgr_get()->LayerCount_get(); ++i)
		{
			MacImage* image = application.LayerMgr_get()->Layer_get(i);
			imageList[i] = new Image();
			MacImageToImage(*image, *imageList[i]);
			
		}
		loader.SaveMultiLayer(imageList, application.LayerMgr_get()->LayerCount_get(), dst);
		for (int i = 0; i < application.LayerMgr_get()->LayerCount_get(); ++i)
			delete imageList[i];
		delete[] imageList;
	}

	application.ImageDeactivate();
}

static void RenderPictureCb(PlaybackState* state)
{
	RenderPictureState* renderState = (RenderPictureState*)state->tag;

	if (true)
	{
		if (renderState->preview || (state->packetIndex % FRAME_SKIP == 0))
		{
			Preview(state->area, state->application, state->screen, renderState->preview, state->packetCount, state->packetIndex, state->showProgress);

			SDL_Flip(state->screen);
		}
	}
}

static void RenderPlaybackCb(PlaybackState* state);

class RenderPlaybackState
{
public:
	int frameCount;
};

static void RenderPlayback(FileArchive* archive, int frameMS, bool showProgress)
{
	Application application;

	CreateApplication(application, ImageId(), 3, 320, 480, false, false, archive);

	RenderPlaybackState state;
	state.frameCount = 0;

	Playback(&application, archive, true, frameMS, showProgress, RenderPlaybackCb, &state);

	application.ImageDeactivate();
}

static void RenderPlaybackCb(PlaybackState* state)
{
	RenderPlaybackState* renderState = (RenderPlaybackState*)state->tag;

	Preview(state->area, state->application, state->screen, true, state->packetCount, state->packetIndex, state->showProgress);

	SDL_Flip(state->screen);

	renderState->frameCount++;

	if ((renderState->frameCount % 100) == 0)
	{
		LOG_INF("frame: %d", renderState->frameCount);
	}
}

static void InteractivePaint(SDL_Surface* screen)
{
	printf("commands:\n");
	printf("1 = select brush, soft (random settings)\n");
	printf("2 = select brush, pattern (random settings)\n");
	printf("3 = select smudge, soft (random settings)\n");
	printf("4 = select smudge, pattern (random settings)\n");
	printf("5 = select eraser, soft (random settings)\n");
	printf("6 = select eraser, pattern (random settings)\n");
	printf("7 = select brush, soft, direct (random settings)\n");
	printf("8 = select brush, pattern, direct (random settings)\n");
	printf("F1 = select layer 1\n");
	printf("F2 = select layer 2\n");
	printf("F3 = select layer 3\n");
	printf("- = undo\n");
	printf("= = redo\n");
	printf("q = select color (random)\n");
	printf("ESC = exit\n");

	Application application;

	const ImageId imageId = application.AllocateImageId();

	CreateApplication(application, imageId, DEFAULT_LAYERCOUNT, DEFAULT_SX, DEFAULT_SY, true, true, 0);

	MouseMgr mouseMgr;

	mouseMgr.OnTouchBegin = CallBack(&application, HandleTouchBegin);
	mouseMgr.OnTouchEnd = CallBack(&application, HandleTouchEnd);
	mouseMgr.OnTouchMove = CallBack(&application, HandleTouchMove);

	bool stop = false;

	while (!stop)
	{
		SDL_Event e;

		while (SDL_PollEvent(&e))
		{
			if (e.type == SDL_KEYDOWN)
			{
				if (e.key.keysym.sym == SDLK_1)
				{
					ToolSettings_BrushSoft settings(
						Calc::Random(30) * 2 + 1,
						Calc::RandomMin0Max1(),
						Calc::RandomMin0Max1());
					application.ToolSelect_BrushSoft(settings);
				}
				if (e.key.keysym.sym == SDLK_2)
				{
					ToolSettings_BrushPattern settings(
						Calc::Random(30) * 2 + 1,
						1000,
						Calc::RandomMin0Max1());
					application.ToolSelect_BrushPattern(settings);
				}
				if (e.key.keysym.sym == SDLK_3)
				{
					ToolSettings_SmudgeSoft settings(
						Calc::Random(30) * 2 + 1,
						Calc::RandomMin0Max1(),
						Calc::RandomMin0Max1(),
						Calc::RandomMin0Max1());
					application.ToolSelect_SmudgeSoft(settings);
				}
				if (e.key.keysym.sym == SDLK_4)
				{
					ToolSettings_SmudgePattern settings(
						31,
						1000,
						Calc::RandomMin0Max1(),
						Calc::RandomMin0Max1());
					application.ToolSelect_SmudgePattern(settings);
				}
				if (e.key.keysym.sym == SDLK_5)
				{
					ToolSettings_EraserSoft settings(
						Calc::Random(30) * 2 + 1,
						Calc::RandomMin0Max1(),
						Calc::RandomMin0Max1());
					application.ToolSelect_EraserSoft(settings);
				}
				if (e.key.keysym.sym == SDLK_6)
				{
					ToolSettings_EraserPattern settings(
						Calc::Random(30) * 2 + 1,
						1000,
						Calc::RandomMin0Max1());
					application.ToolSelect_EraserPattern(settings);
				}
				if (e.key.keysym.sym == SDLK_7)
				{
					ToolSettings_BrushSoftDirect settings(
						Calc::Random(30) * 2 + 1,
						Calc::RandomMin0Max1(),
						Calc::RandomMin0Max1());
					application.ToolSelect_BrushSoftDirect(settings);
				}
				if (e.key.keysym.sym == SDLK_8)
				{
					ToolSettings_BrushPatternDirect settings(
						Calc::Random(30) * 2 + 1,
						1000,
						Calc::RandomMin0Max1());
					application.ToolSelect_BrushPatternDirect(settings);
				}
				if (e.key.keysym.sym == SDLK_F1)
				{
					application.DataLayerSelect(0);
				}
				if (e.key.keysym.sym == SDLK_F2)
				{
					application.DataLayerSelect(1);
				}
				if (e.key.keysym.sym == SDLK_F3)
				{
					application.DataLayerSelect(2);
				}
				if (e.key.keysym.sym == SDLK_q)
				{
					application.ColorSelect(
						Calc::RandomMin0Max1(),
						Calc::RandomMin0Max1(),
						Calc::RandomMin0Max1(),
						Calc::RandomMin0Max1());
				}
				if (e.key.keysym.sym == SDLK_w)
				{
					application.ColorSelect(1.0f, 1.0f, 0.0f, 0.1f);
				}
				if (e.key.keysym.sym == SDLK_e)
				{
					application.ColorSelect(0.0f, 1.0f, 1.0f, 0.1f);
				}
				if (e.key.keysym.sym == SDLK_a)
				{
					application.ColorSelect(
						Calc::RandomMin0Max1(),
						Calc::RandomMin0Max1(),
						Calc::RandomMin0Max1(),
						0.01f);
				}
				if (e.key.keysym.sym == SDLK_s)
				{
					//application.LayerSwap(0, 1);
				}
				if (e.key.keysym.sym == SDLK_m)
				{
					application.DataLayerMerge(0, 1);
				}
				if (e.key.keysym.sym == SDLK_c)
				{
					application.DataLayerClear(application.LayerMgr_get()->ActiveDataLayer_get(), Calc::Random(1.0f), Calc::Random(1.0f), Calc::Random(1.0f), Calc::Random(1.0f));
				}
				if (e.key.keysym.sym == SDLK_p)
				{
#if 0
					commandList = application.DBG_GetCommandList();
					for (size_t i = 0; i < commandList.size(); ++i)
					{
						CommandPacket packet = commandList[i];
						if (packet.mCommandType == CommandType_ToolSelect_PatternBrush)
							//packet.brush_pattern.diameter = ((packet.brush_pattern.diameter - 1) * 2) + 1;
							packet.brush_pattern.diameter *= 2;
						if (packet.mCommandType == CommandType_ToolSelect_SoftBrush)
							//packet.brush_soft.diameter = ((packet.brush_soft.diameter - 1) * 2) + 1;
							packet.brush_soft.diameter *= 2;
						if (packet.mCommandType == CommandType_ImageSize)
						{
							packet.image_size.sx *= 2;
							packet.image_size.sy *= 2;
						}
						if (packet.mCommandType == CommandType_ToolSelect_PatternSmudge)
							//packet.smudge_pattern.diameter = ((packet.smudge_pattern.diameter - 1) * 2) + 1;
							packet.smudge_pattern.diameter *= 2;
						if (packet.mCommandType == CommandType_ToolSelect_SoftSmudge)
							//packet.smudge_soft.diameter = ((packet.smudge_soft.diameter - 1) * 2) + 1;
							packet.smudge_soft.diameter *= 2;
						if (packet.mCommandType == CommandType_ToolSelect_PatternErazor)
							packet.erazor_pattern.diameter *= 2;
						if (packet.mCommandType == CommandType_Stroke_Begin)
						{
							packet.stroke_begin.x *= 2;
							packet.stroke_begin.y *= 2;
						}
						if (packet.mCommandType == CommandType_Stroke_Move)
						{
							packet.stroke_move.x *= 2;
							packet.stroke_move.y *= 2;
						}
						commandList[i] = packet;
					}
					application.ImageStart(application.ImageId_get());
#endif
				}
				if (e.key.keysym.sym == SDLK_MINUS)
				{
					application.Undo();
				}
				if (e.key.keysym.sym == SDLK_EQUALS)
				{
					application.Redo();
				}
				if (e.key.keysym.sym == SDLK_LEFTBRACKET)
				{
					// move layer down

					//int index = application.LayerMgr_get()->ActiveDataLayer_get();

					//if (layer - 1 >= 0)
					//	application.LayerSwap(layer, layer - 1);
				}
				if (e.key.keysym.sym == SDLK_RIGHTBRACKET)
				{
					// move layer up

					//int index = application.LayerMgr_get()->ActiveDataLayer_get();

					//if (layer + 1 < application.LayerMgr_get()->LayerCount_get())
					//	application.LayerSwap(layer, layer + 1);
				}
				if (e.key.keysym.sym == SDLK_ESCAPE)
				{
					stop = true;
				}
			}
			
			mouseMgr.Update(&e);
		}

		AreaI area = application.LayerMgr_get()->Validate();

		if (area.IsSet_get())
		{
			Preview(area, &application, screen, true, 0, 0, false);

			SDL_Flip(screen);
		}
		else
		{
			//if (commandList.size() == 0)
				SDL_Delay(10);
		}
	}

	application.ImageDeactivate();

	application.SaveImage();

	FileStream stream;

	stream.Open("999.klodder", OpenMode_Write);

	Application::SaveImageArchive(application.ImageId_get(), &stream);

	stream.Close();
}

int main(int argc, char* argv[])
{
	try
	{
		Calc::Initialize();

#ifdef DEBUG
		//TestQuickTime();
		//TestExportPsd();
		//TestXml();
#endif

		if (argc == 1)
		{
			printf("usage:\n");
			printf("klodder -m replay -c <source> -s <scale> -b1 <r> <g> <b> -b2 <r> <g> <b>\n");
			printf("klodder -m picture -c <source> -o <output> -s <scale> -b1 <r> <g> <b> -b2 <r> <g> <b> -p <preview>\n");
			printf("klodder -m movie -c <source> -o <output> -s <scale> -b1 <r> <g> <b> -b2 <r> <g> <b> -fps <fps> -mt <trail> -p <preview>\n");
			printf("\n");
			printf("-m:\tRequest type (replay, picture, movie)\n");
			printf("-c:\tThe input klodder file to render\n");
			printf("-o:\tThe output destination file\n");
			printf("-s:\tThe amount of upsampling to use: 1..4\n");
			printf("-b1:\tThe first background color. r/g/b must be between 0..255\n");
			printf("-b2:\tThe second background color. r/g/b must be between 0..255\n");
			printf("-fps:\tThe movie or replay frame rate (Hz)\n");
			printf("-mt:\tThe number of repeated final frames at the end of the movie\n");
			printf("-p:\tPreview rendered picture during picture or movie export: 0 or 1\n");
			printf("-w:\tWait for ESCAPE keypress after request completed\n");

			exit(0);
		}

		for (int i = 1; i < argc;)
		{
			if (!strcmp(argv[i], "-o"))
			{
				ARGS_CHECKPARAM(1);

				sSettings.dst = argv[i + 1];

				i += 2;
			}
			else if (!strcmp(argv[i], "-s"))
			{
				ARGS_CHECKPARAM(1);

				sSettings.scale = Parse::Int32(argv[i + 1]);

				i += 2;
			}
			else if (!strcmp(argv[i], "-b1"))
			{
				ARGS_CHECKPARAM(3);

				sSettings.backColor1.rgb[0] = Parse::Int32(argv[i + 1]) / 255.0f;
				sSettings.backColor1.rgb[1] = Parse::Int32(argv[i + 2]) / 255.0f;
				sSettings.backColor1.rgb[2] = Parse::Int32(argv[i + 3]) / 255.0f;

				i += 4;
			}
			else if (!strcmp(argv[i], "-b2"))
			{
				ARGS_CHECKPARAM(3);

				sSettings.backColor2.rgb[0] = Parse::Int32(argv[i + 1]) / 255.0f;
				sSettings.backColor2.rgb[1] = Parse::Int32(argv[i + 2]) / 255.0f;
				sSettings.backColor2.rgb[2] = Parse::Int32(argv[i + 3]) / 255.0f;

				i += 4;
			}
			else if (!strcmp(argv[i], "-p"))
			{
				ARGS_CHECKPARAM(1);

				sSettings.preview = Parse::Bool(argv[i + 1]);

				i += 2;
			}
			else if (!strcmp(argv[i], "-fps"))
			{
				ARGS_CHECKPARAM(1);

				int fps = Parse::Int32(argv[i + 1]);

				if (fps != 0)
					sSettings.movieFps = fps;

				sSettings.replayFps = fps;

				i += 2;
			}
			else if (!strcmp(argv[i], "-m"))
			{
				ARGS_CHECKPARAM(1);

				std::string type = argv[i + 1];

				if (type == "replay")
					sSettings.requestType = RequestType_Replay;
				else if (type == "movie")
					sSettings.requestType = RequestType_RenderToMovie;
				else if (type == "picture")
					sSettings.requestType = RequestType_RenderToPicture;
				else if (type == "paint")
					sSettings.requestType = RequestType_Paint;
				else
					throw ExceptionVA("unknown request type: %s", type.c_str());

				i += 2;
			}
			else if (!strcmp(argv[i], "-mt"))
			{
				ARGS_CHECKPARAM(1);

				sSettings.movieTrail = Parse::Int32(argv[i + 1]);

				i += 2;
			}
			else if (!strcmp(argv[i], "-w"))
			{
				sSettings.waitKey = true;

				i += 1;
			}
			else
			{
				sSettings.src = argv[i];

				i += 1;
			}
		}

#ifdef DEBUG
		printf("mode:\n");
		printf("[p]aint\n");
		printf("[r]replay\n");
		printf("[i]mage\n");
		printf("[m]movie\n");

		const char mode = getchar();

		switch (mode)
		{
		case 'p':
			sSettings.requestType = RequestType_Paint;
			break;
		case 'r':
			sSettings.requestType = RequestType_Replay;
			break;
		case 'i':
			sSettings.requestType = RequestType_RenderToPicture;
			break;
		case 'm':
			sSettings.requestType = RequestType_RenderToMovie;
			break;
		}
#endif

		sSettings.Validate();

		//

		LOG_INF("initializing SDL", 0);

		SDL_putenv((char*)"SDL_VIDEO_CENTERED=center");

		const uint32_t mask = SDL_INIT_TIMER | SDL_INIT_VIDEO;

		SDL_Init(mask);

		SDL_WM_SetCaption("Klodder Work-Horse", 0);

		LOG_INF("initializing SDL [done]", 0);

		switch (sSettings.requestType)
		{
		case RequestType_Paint:
			{
				LOG_INF("creating SDL window", 0);

				const int displaySx = gSettings.GetInt32("display_sx", 320);
				const int displaySy = gSettings.GetInt32("display_sy", 480);

				const int screenFlags = SDL_SWSURFACE;

				SDL_Surface* screen = SDL_SetVideoMode(displaySx, displaySy, 32, screenFlags);

				LOG_INF("creating SDL window [done]", 0);

				InteractivePaint(screen);

				SDL_FreeSurface(screen);
				break;
			}
		case RequestType_RenderToMovie:
			{
				FileStream stream;
				stream.Open(sSettings.src.c_str(), OpenMode_Read);

				FileArchive archive;
				archive.Load(&stream);

#if USE_QUICKTIME
				RenderMovie(&archive, sSettings.dst, sSettings.movieFps, sSettings.preview, sSettings.showProgress, sSettings.movieTrail);
#else
				throw ExceptionVA("QuickTime support is disabled in this build.");
#endif
				break;
			}
		case RequestType_RenderToPicture:
			{
				FileStream stream;
				stream.Open(sSettings.src.c_str(), OpenMode_Read);

				FileArchive archive;
				archive.Load(&stream);

				RenderPicture(&archive, sSettings.dst, sSettings.preview, sSettings.showProgress);
				break;
			}
		case RequestType_Replay:
			{
				FileStream stream;
				stream.Open(sSettings.src.c_str(), OpenMode_Read);

				FileArchive archive;
				archive.Load(&stream);

				int frameMS = sSettings.replayFps > 0 ? 1000 / sSettings.replayFps : 0;

				RenderPlayback(&archive, frameMS, sSettings.showProgress);
				break;
			}
		}

		if (sSettings.waitKey)
		{
			SDL_Event e;

			bool stop = false;

			while (SDL_WaitEvent(&e) && !stop)
			{
				if (e.type == SDL_KEYDOWN)
				{
					if (e.key.keysym.sym == SDLK_ESCAPE)
					{
						stop = true;
					}
				}
			}
		}

		return 0;
	}
	catch (std::exception& e)
	{
		printf("error: %s\n", e.what());

		return -1;
	}
}

static void HandleTouchBegin(void* obj, void* arg)
{
	Application* app = (Application*)obj;
	TouchInfo* ti = (TouchInfo*)arg;

	app->StrokeBegin(app->LayerMgr_get()->ActiveDataLayer_get(), true, false, ti->m_Location[0], ti->m_Location[1]);
}

static void HandleTouchEnd(void* obj, void* arg)
{
	Application* app = (Application*)obj;
//	TouchInfo* ti = (TouchInfo*)arg;

	app->StrokeEnd();
}

static void HandleTouchMove(void* obj, void* arg)
{
	Application* app = (Application*)obj;
	TouchInfo* ti = (TouchInfo*)arg;

	app->StrokeMove(ti->m_Location[0], ti->m_Location[1]);
}

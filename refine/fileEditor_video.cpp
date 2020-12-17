#include "fileEditor_video.h"
#include "framework.h"
#include "imgui.h" // doButtonBar
#include "reflection.h"
#include "ui.h" // drawUiRectCheckered
#include "video.h"

static void doProgressBar(const int x, const int y, const int sx, const int sy, const double time, const double duration, const float opacity, bool & hover, bool & seek, double & seekTime);

FileEditor_Video::FileEditor_Video(const char * path)
{
	MediaPlayer::OpenParams openParams;
	openParams.enableAudioStream = true;
	openParams.enableVideoStream = true;
	openParams.filename = path;
	openParams.outputMode = MP::kOutputMode_RGBA;
	openParams.audioOutputMode = MP::kAudioOutputMode_Stereo;
	
	mp.openAsync(openParams);
}

FileEditor_Video::~FileEditor_Video()
{
	audioOutput.Shutdown();
	
	mp.close(true);
}

bool FileEditor_Video::reflect(TypeDB & typeDB, StructuredType & type)
{
	typeDB.addEnum<FileEditor_Video::SizeMode>("FileEditor_Video::SizeMode")
		.add("contain", kSizeMode_Contain)
		.add("fill", kSizeMode_Fill)
		.add("dontScale", kSizeMode_DontScale);
	
	type.add("sizeMode", &FileEditor_Video::sizeMode);
	
	return true;
}

void FileEditor_Video::tick(const int sx, const int sy, const float dt, const bool hasFocus, bool & inputIsCaptured)
{
	// attempt to get information about the video so we can start the audio

	int channelCount;
	int sampleRate;
	
	if (hasAudioInfo == false && mp.getAudioProperties(channelCount, sampleRate))
	{
		hasAudioInfo = true;

		if (channelCount > 0)
		{
			audioIsStarted = true;
		
			audioOutput.Initialize(channelCount, sampleRate, 256);
			audioOutput.Play(&mp);
		}
	}
	
	// update presentation time stamp for the video

	if (audioIsStarted)
		mp.presentTime = mp.audioTime;
	else if (hasAudioInfo)
		mp.presentTime += dt;
	
	// update fill mode
	
	if (inputIsCaptured == false && keyboard.wentDown(SDLK_RETURN))
	{
		sizeMode = SizeMode((sizeMode + 1) % kSizeMode_COUNT);
	}
	
	// update video

	mp.tick(mp.context, true);

	int videoSx;
	int videoSy;
	double duration;
	double sampleAspectRatio;

	const bool hasVideoInfo = mp.getVideoProperties(videoSx, videoSy, duration, sampleAspectRatio);

	// check if the video ended and needs to be looped

	if (mp.presentedLastFrame(mp.context))
	{
		auto openParams = mp.context->openParams;

		mp.close(false);
		mp.presentTime = 0.0;
		mp.openAsync(openParams);

		audioOutput.Shutdown();

		hasAudioInfo = false;
		audioIsStarted = false;
	}
	
	// update progress bar visibility
	
	if (inputIsCaptured || mouse.isIdle())
		progressBarTimer -= dt / 3.f;
	else
		progressBarTimer = 1.f;
	
	//
	
	clearSurface(0, 0, 0, 0);
	
	pushBlend(BLEND_OPAQUE);
	setColor(colorWhite);
	drawUiRectCheckered(0, 0, sx, sy, 8);
	popBlend();
	
	auto texture = mp.getTexture();
	
	if (texture != 0 && hasVideoInfo)
	{
		videoSx *= sampleAspectRatio;
		
		pushBlend(BLEND_OPAQUE);
		{
			const float fillScaleX = sx / float(videoSx);
			const float fillScaleY = sy / float(videoSy);
			
			float scale = 1.f;
			
			if (sizeMode == kSizeMode_Fill)
			{
				scale = fmaxf(fillScaleX, fillScaleY);
			}
			else if (sizeMode == kSizeMode_Contain)
			{
				scale = fminf(fillScaleX, fillScaleY);
			}
			else if (sizeMode == kSizeMode_DontScale)
			{
				scale = 1.f;
			}
			else
			{
				Assert(false);
			}
			
			gxPushMatrix();
			{
				gxTranslatef((sx - videoSx * scale) / 2, (sy - videoSy * scale) / 2, 0);
				gxScalef(scale, scale, 1.f);
				
				gxSetTexture(texture);
				setColor(colorWhite);
				drawRect(0, 0, videoSx, videoSy);
				gxSetTexture(0);
			}
			gxPopMatrix();
		}
		popBlend();
		
		// draw the progress bar

		int videoSx;
		int videoSy;
		double duration;
		double sampleAspectRatio;

		if (mp.getVideoProperties(videoSx, videoSy, duration, sampleAspectRatio))
		{
			const float opacity = saturate(progressBarTimer / (1.f - .6f));
			
			bool hover = false;
			bool seek = false;
			double seekTime;
			
			doProgressBar(20, sy-20-20, sx-20-20, 20, mp.presentTime, duration, opacity, hover, seek, seekTime);
			
			if (seek)
			{
				const bool nearest = true;
				
				mp.seek(seekTime, nearest);
			
				framework.process();
				framework.process();
			}
		}
	}
}

static void doProgressBar(const int x, const int y, const int sx, const int sy, const double time, const double duration, const float opacity, bool & hover, bool & seek, double & seekTime)
{
	// tick
	
	hover =
		mouse.x >= x &&
		mouse.y >= y &&
		mouse.x < x + sx &&
		mouse.y < y + sy;
	
	if (hover && (mouse.wentDown(BUTTON_LEFT) || (keyboard.isDown(SDLK_LCTRL) && mouse.isDown(BUTTON_LEFT))))
	{
		seek = true;
		seekTime = clamp((mouse.x - x) / double(sx) * duration, 0.0, duration);
	}
	
	// draw
	
	const double t = time / duration;
	
	setColor(63, 127, 255, 127 * opacity);
	hqBegin(HQ_FILLED_ROUNDED_RECTS);
	hqFillRoundedRect(x, y, x + sx * t, y + sy, 6);
	hqEnd();
	
	setColor(63, 127, 255, 255 * opacity);
	hqBegin(HQ_STROKED_ROUNDED_RECTS);
	hqStrokeRoundedRect(x, y, x + sx, y + sy, 6, 2);
	hqEnd();
	
	setFont("calibri.ttf");
	pushFontMode(FONT_SDF);
	setColorf(1.f, 1.f, 1.f, opacity);
	
	const int hours = int(floor(time / 3600.0));
	const int minutes = int(floor(fmod(time / 60.0, 60.0)));
	const int seconds = int(floor(fmod(time, 60.0)));
	const int hundreds = int(floor(fmod(time, 1.0) * 100.0));
	
	const int d_hours = int(floor(duration / 3600.0));
	const int d_minutes = int(floor(fmod(duration / 60.0, 60.0)));
	const int d_seconds = int(floor(fmod(duration, 60.0)));
	const int d_hundreds = int(floor(fmod(duration, 1.0) * 100.0));
	
	drawText(x + 10, y + sy/2, 12, +1, 0, "%02d:%02d:%02d.%02d / %02d:%02d:%02d.%02d", hours, minutes, seconds, hundreds, d_hours, d_minutes, d_seconds, d_hundreds);
	popFontMode();
}

void FileEditor_Video::doButtonBar()
{
	if (ImGui::BeginMenu("Scale"))
	{
		if (ImGui::MenuItem("Contain", nullptr, sizeMode == kSizeMode_Contain))
			sizeMode = kSizeMode_Contain;
		if (ImGui::MenuItem("Fill", nullptr, sizeMode == kSizeMode_Fill))
			sizeMode = kSizeMode_Fill;
		if (ImGui::MenuItem("Don't scale", nullptr, sizeMode == kSizeMode_DontScale))
			sizeMode = kSizeMode_DontScale;
		
		ImGui::EndMenu();
	}
}

/*
	Copyright (C) 2017 Marcel Smit
	marcel303@gmail.com
	https://www.facebook.com/marcel.smit981

	Permission is hereby granted, free of charge, to any person
	obtaining a copy of this software and associated documentation
	files (the "Software"), to deal in the Software without
	restriction, including without limitation the rights to use,
	copy, modify, merge, publish, distribute, sublicense, and/or
	sell copies of the Software, and to permit persons to whom the
	Software is furnished to do so, subject to the following
	conditions:

	The above copyright notice and this permission notice shall be
	included in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
	OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
	NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
	HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
	WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
	OTHER DEALINGS IN THE SOFTWARE.
*/

#include "audiooutput/AudioOutput_PortAudio.h"
#include "DownloadCache.h"
#include "framework.h"
#include "StringEx.h"
#include "video.h"

#define ENABLE_ARTNET 0 // when enabled, the averaged smoothed RGB color of the video is sent to DMX channels 0..2 to the host defined below

#if ENABLE_ARTNET
	#include "artnet.h"
	#include "ip/UdpSocket.h"

	#include "mediaplayer/MPVideoBuffer.h"

	#define HOST_IP "192.168.1.220"
	#define HOST_PORT 6454
#endif

static void doProgressBar(const int x, const int y, const int sx, const int sy, const double time, const double duration, const float opacity, bool & hover, bool & seek, double & seekTime);
static void downloadMediaFiles();

int main(int argc, char * argv[])
{
#if defined(CHIBI_RESOURCE_PATH)
	changeDirectory(CHIBI_RESOURCE_PATH);
#else
	changeDirectory(SDL_GetBasePath());
#endif

#if ENABLE_ARTNET
	IpEndpointName endpointName(HOST_IP, HOST_PORT);
	UdpTransmitSocket transmitSocket(endpointName);
#endif

	framework.filedrop = true;
	framework.windowIsResizable = true;
	
	if (framework.init(800, 400))
	{
		downloadMediaFiles();

		MediaPlayer mp;
		
		MediaPlayer::OpenParams openParams;
		openParams.filename = "newpath.mp4";
		openParams.outputMode = MP::kOutputMode_RGBA;
		openParams.enableAudioStream = true;
		openParams.enableVideoStream = true;
		openParams.desiredAudioStreamIndex = 1; // select second audio stream, if available
		mp.openAsync(openParams);
		
		AudioOutput_PortAudio audioOutput;
		static AudioOutput_PortAudio & s_audioOutput = audioOutput;
		
		bool hasAudioInfo = false;
		static bool & s_hasAudioInfo = hasAudioInfo;
		
		static MediaPlayer & s_mp = mp;
		
		framework.actionHandler = [](const std::string & action, const Dictionary & d)
		{
			if (action == "filedrop")
			{
				// close the video
				s_mp.close(true);
				s_mp.presentTime = 0.0;
				
				// close the audio output
				s_audioOutput.Shutdown();
				s_hasAudioInfo = false;
				
				// open the new file
				
				auto filename = d.getString("file", "");
				
				auto openParams = s_mp.context->openParams;
				openParams.filename = filename;
				
				s_mp.openAsync(openParams);
			}
		};

		SDL_Cursor * handCursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND);
		
		int channelCount = 0;
		int sampleRate = 0;

		float progressBarTimer = 0.f;
		
		std::string osdText;
		float osdTimer = 0.f;
		
		double videoTimeOffset = 0.0;
		
	#if ENABLE_ARTNET
		Color averageColor = colorBlack;
	#endif
		
		while (!framework.quitRequested)
		{
			framework.process();
			
			SDL_Cursor * cursor = SDL_GetDefaultCursor();

			if (keyboard.wentDown(SDLK_ESCAPE))
				framework.quitRequested = true;

			if (keyboard.wentDown(SDLK_1))
			{
				if (keyboard.isDown(SDLK_LCTRL))
					videoTimeOffset = 0.0;
				else if (keyboard.isDown(SDLK_LSHIFT))
					videoTimeOffset -= 10.0 / 1000.0;
				else
					videoTimeOffset += 10.0 / 1000.0;
				
				videoTimeOffset = clamp<double>(videoTimeOffset, -0.5, +0.5);
				
				osdText = String::FormatC("Video time offset %dms", int(round(videoTimeOffset * 1000.0)));
				osdTimer = 1.f;
			}
			
			if (mouse.isIdle())
				progressBarTimer -= framework.timeStep / 3.f;
			else
				progressBarTimer = 1.f;

			if (progressBarTimer < 0.f)
				mouse.showCursor(false);
			else
				mouse.showCursor(true);
			
			osdTimer -= framework.timeStep / 3.f;
			
			if (hasAudioInfo == false)
			{
				// try to fetch audio properties and start audio (if audio is available)
				
				Assert(audioOutput.IsPlaying_get() == false);
				
				if (mp.getAudioProperties(channelCount, sampleRate))
				{
					hasAudioInfo = true;
					
					// does the video have audio ? if so start audio output
					
					if (channelCount > 0)
					{
						audioOutput.Initialize(channelCount, sampleRate, 256);
						audioOutput.Play(&mp);
					}
				}
			}
			
			if (hasAudioInfo)
			{
				// use the current audio time if the video has audio. otherwise, increment the presentation time stamp based on the elapsed time
				
				if (audioOutput.IsPlaying_get())
					mp.presentTime = mp.audioTime + videoTimeOffset;
				else
					mp.presentTime += framework.timeStep;
			}
			
			mp.tick(mp.context, true);
			
		#if ENABLE_ARTNET
			if (mp.videoFrame != nullptr)
			{
				int sx;
				int sy;
				int pitch;
				
				const uint8_t * rgba = mp.videoFrame->getRGBA(sx, sy, pitch);
				
				int64_t totalR = 0;
				int64_t totalG = 0;
				int64_t totalB = 0;
				
				int num = 0;
				
				if (rgba != nullptr)
				{
					// compute average color
					
					for (int y = 0; y < sy; y += 4)
					{
						const uint8_t * line = rgba + y * pitch;
						
						for (int x = 0; x < sx; x += 4)
						{
							totalR += line[x * 4 + 0];
							totalG += line[x * 4 + 1];
							totalB += line[x * 4 + 2];
							
							num++;
						}
					}
					
					averageColor = Color(
						totalR / num,
						totalG / num,
						totalB / num,
						255);
					
					static float t = 0.f;
					t += framework.timeStep;
					
					if (t > 1.f / 50.f)
					{
						static float average[3] =
						{
							0.f, 0.f, 0.f
						};
						
						const int64_t total[3] =
						{
							totalR,
							totalG,
							totalB
						};
						
						for (int i = 0; i < 3; ++i)
						{
							const float value = total[i] / float(num);
							
							average[i] = average[i] * .98f + value * .02f;
						}
						
						const uint8_t values[4] =
						{
							uint8_t(1 + average[0] / 1.01f),
							uint8_t(1 + average[1] / 1.01f),
							uint8_t(1 + average[2] / 1.01f),
							0
						};
						
						ArtnetPacket packet;
						packet.makeDMX512(values, 4);
						transmitSocket.Send((const char*)packet.data, packet.dataSize);
						
						t = 0.f;
					}
				}
			}
		#endif
			
			if (mp.presentedLastFrame(mp.context))
			{
				auto openParams = mp.context->openParams;
				mp.close(false);
				mp.presentTime = 0.0;
				mp.openAsync(openParams);
			}
			
			framework.beginDraw(0, 0, 0, 0);
			{
				int viewportSx;
				int viewportSy;
				framework.getCurrentViewportSize(viewportSx, viewportSy);
				
				int sx;
				int sy;
				double duration;
				double sampleAspectRatio;
				
				const bool hasVideoInfo = mp.getVideoProperties(sx, sy, duration, sampleAspectRatio);
				
				// draw the video frame
				
				const GxTextureId texture = mp.getTexture();

				if (texture != 0 && hasVideoInfo)
				{
					sx *= sampleAspectRatio;
					
					const float scaleX = viewportSx / float(sx);
					const float scaleY = viewportSy / float(sy);
					const float scale = fminf(scaleX, scaleY);
					const float scaledSx = sx * scale;
					const float scaledSy = sy * scale;
					const float offsetX = (viewportSx - scaledSx) / 2.f;
					const float offsetY = (viewportSy - scaledSy) / 2.f;
					
					gxPushMatrix();
					{
						gxTranslatef(offsetX, offsetY, 0.f);
						gxScalef(scale, scale, 1);
						
						setColor(colorWhite);
						gxSetTexture(texture);
						pushBlend(BLEND_OPAQUE);
						drawRect(0, 0, sx, sy);
						popBlend();
						gxSetTexture(0);
					}
					gxPopMatrix();
				}
				
				// draw the progress bar
				
				if (hasVideoInfo)
				{
					bool hover = false;
					bool seek = false;
					double seekTime;
					
					doProgressBar(20, viewportSy-20-20, viewportSx * 4/5, 20, mp.presentTime, duration, saturate(progressBarTimer / (1.f - .6f)), hover, seek, seekTime);
					
					if (hover)
						cursor = handCursor;
					
					if (seek)
					{
						const bool exact =
							keyboard.isDown(SDLK_LSHIFT) ||
							keyboard.isDown(SDLK_RSHIFT);
						
						mp.seek(seekTime, exact == false);
					
						framework.process();
						framework.process();
					}
				}
				
				const float osdOpacity = saturate(osdTimer / (1.f - .6f));
				
				if (osdOpacity > 0.f)
				{
					setColorf(1.f, 1.f, 1.f, osdOpacity);
					drawText(viewportSx / 2, viewportSy / 2, 24, 0, 0, "%s", osdText.c_str());
				}
			}
			framework.endDraw();
			
			SDL_SetCursor(cursor);
		}
		
		audioOutput.Stop();
		audioOutput.Shutdown();
		
		mp.close(true);
		
		framework.shutdown();
	}

	return 0;
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

static void downloadMediaFiles()
{
	const char * url = "http://centuryofthecat.nl/shared_media/framework/libvideo-examples/newpath.mp4";
	const char * filename = "newpath.mp4";

	DownloadCache downloadCache;

	downloadCache.add(url, filename);

	while (downloadCache.downloadQueue.isEmpty() == false)
	{
		framework.process();

		downloadCache.tick(4);

		framework.beginDraw(0, 0, 0, 0);
		{
			setFont("calibri.ttf");
			setColor(colorWhite);
			drawText(400, 200, 16, 0, 0, "Downloading media files..");

			int y = 200 + 20;
			for (auto & e : downloadCache.downloadQueue.activeElems)
			{
				drawText(400, 200 + 20, 14, 0, 0, "Downloading %s/%dkb..", e.first.c_str(), e.second.getProgress());
				y += 18;
			}
		}
		framework.endDraw();
	}
}

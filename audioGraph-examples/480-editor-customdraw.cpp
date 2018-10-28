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

#include "audioGraph.h"
#include "audioGraphManager.h"
#include "audioGraphRealTimeConnection.h"
#include "audioNodeBase.h"
#include "audioUpdateHandler.h"
#include "audioVoiceManager.h"
#include "framework.h"
#include <cmath>

const int GFX_SX = 1024;
const int GFX_SY = 768;

#define CHANNEL_COUNT 16

static void drawChannels(const GraphEdit_ChannelData & channelData, const float sx, const float sy)
{
	for (auto & channel : channelData.channels)
	{
		if (channel.numValues < 2)
			continue;
		
		gxBegin(GL_LINES);
		{
			const float xScale = sx / (channel.numValues - 1);
			const float xOffset = -sx/2.f;
			
			float min = channel.values[0];
			float max = channel.values[0];
			
			for (int i = 1; i < channel.numValues; ++i)
			{
				if (channel.values[i] < min)
					min = channel.values[i];
				else if (channel.values[i] > max)
					max = channel.values[i];
			}
			
			if (min == max)
			{
				const int x1 = 0;
				const int x2 = channel.numValues - 1;
				
				gxVertex2f(x1 * xScale + xOffset, 0.f);
				gxVertex2f(x2 * xScale + xOffset, 0.f);
			}
			else
			{
				const float yScale = - sy / (max - min);
				const float yOffset = - (min + max) / 2.f * yScale;
				
				for (int i = 0; i < channel.numValues - 1; ++i)
				{
					gxVertex2f((i + 0) * xScale + xOffset, channel.values[i + 0] * yScale + yOffset);
					gxVertex2f((i + 1) * xScale + xOffset, channel.values[i + 1] * yScale + yOffset);
				}
			}
		}
		gxEnd();
	}
}

static void drawEditor(const GraphEdit & graphEdit, AudioRealTimeConnection * rtc, AudioGraph * audioGraph)
{
	const float scale = .01f;
	const float kRadius = 40.f;
	
	// todo : would be nice to have an axis swizzle function
	gxScalef(scale, scale, scale);
	gxRotatef(180, 1, 0, 0);
	
	// draw links
	
	for (auto & linkItr : graphEdit.graph->links)
	{
		auto linkId = linkItr.first;
		auto link = graphEdit.tryGetLink(linkId);
		
		if (link)
		{
			auto srcNodeData = graphEdit.tryGetNodeData(link->srcNodeId);
			auto dstNodeData = graphEdit.tryGetNodeData(link->dstNodeId);
			
			if (srcNodeData && dstNodeData)
			{
				const float dx = dstNodeData->x - srcNodeData->x;
				const float dy = dstNodeData->y - srcNodeData->y;
				const float ds = std::hypot(dx, dy);
				const float nx = dx / ds;
				const float ny = dy / ds;
				
				setColor(colorGreen);
				drawLine(
					srcNodeData->x + nx * kRadius,
					srcNodeData->y + ny * kRadius,
					dstNodeData->x - nx * kRadius,
					dstNodeData->y - ny * kRadius);
			}
		}
	}
	
	// draw nodes
	
	for (auto & nodeItr : graphEdit.graph->nodes)
	{
		auto nodeId = nodeItr.first;
		auto node = graphEdit.tryGetNode(nodeId);
		auto nodeData = graphEdit.tryGetNodeData(nodeId);
		
		if (node && nodeData)
		{
			const float x = std::sin(nodeData->x + nodeData->y + framework.time * 1.12f) * 4.f;
			const float y = std::sin(nodeData->x + nodeData->y + framework.time * 1.23f) * 4.f;
			const float z = std::sin(nodeData->x + nodeData->y + framework.time * 1.45f) * 11.f;
			
			gxPushMatrix();
			gxTranslatef(nodeData->x + x, nodeData->y + y, z);
			
			setColor(20, 20, 20);
			fillCircle(0, 0, kRadius, 100);
			
			gxTranslatef(0, 0, 1.f);
			
			auto typeDefinition = graphEdit.typeDefinitionLibrary->tryGetTypeDefinition(node->typeName.c_str());
			
			if (typeDefinition)
			{
			#if 1
				for (auto & inputSocket : typeDefinition->inputSockets)
				{
					if (inputSocket.typeName == "audioValue")
					{
						GraphEdit_ChannelData channelData;
						
						if (rtc->getSrcSocketChannelData(nodeId, inputSocket.index, inputSocket.name, channelData))
						{
							setColor(colorRed);
							drawChannels(channelData, 30.f, 30.f);
						}
						
						break;
					}
				}
			#endif
			
			#if 1
				for (auto & outputSocket : typeDefinition->outputSockets)
				{
					if (outputSocket.typeName == "audioValue")
					{
						GraphEdit_ChannelData channelData;
						
						if (rtc->getDstSocketChannelData(nodeId, outputSocket.index, outputSocket.name, channelData))
						{
							setColor(colorGreen);
							drawChannels(channelData, 30.f, 30.f);
						}
						
						break;
					}
				}
			#endif
			
			#if 1
				auto audioNodeItr = audioGraph->nodes.find(nodeId);
				
				if (audioNodeItr != audioGraph->nodes.end())
				{
					AudioNodeBase * audioNode = audioNodeItr->second;
					
					const int numSteps = 256;
					float magnitude[numSteps];
					
					if (audioNode->getFilterResponse(magnitude, numSteps))
					{
						gxBegin(GL_LINES);
						{
							const float sx = 30.f;
							const float sy = 30.f;
							
							const float xScale = sx / (numSteps - 1);
							const float xOffset = -sx/2.f;
							
							float min = 0.f;
							float max = 1.f;
							
						#if 0
							for (int i = 1; i < numSteps ++i)
							{
								if (channel.values[i] < min)
									min = channel.values[i];
								else if (channel.values[i] > max)
									max = channel.values[i];
							}
							
							if (min == max)
							{
								const int x1 = 0;
								const int x2 = channel.numValues - 1;
								
								gxVertex2f(x1 * xScale + xOffset, 0.f);
								gxVertex2f(x2 * xScale + xOffset, 0.f);
							}
							else
						#endif
							{
								const float yScale = - sy / (max - min);
								const float yOffset = - (min + max) / 2.f * yScale;
								
								for (int i = 0; i < numSteps - 1; ++i)
								{
									gxVertex2f((i + 0) * xScale + xOffset, magnitude[i + 0] * yScale + yOffset);
									gxVertex2f((i + 1) * xScale + xOffset, magnitude[i + 1] * yScale + yOffset);
								}
							}
						}
						gxEnd();
					}
				}
			#endif
			}
			
			setColor(100, 100, 100);
			for (int i = 0; i < 10; ++i)
			drawCircle(0, 0, kRadius+i, 100);
			
			setColor(200, 200, 200);
			drawText(0, kRadius - 20, 10, 0, 0, "%s", node->typeName.c_str());
			
			gxPopMatrix();
		}
	}
}

int main(int argc, char * argv[])
{
#if defined(CHIBI_RESOURCE_PATH)
	changeDirectory(CHIBI_RESOURCE_PATH);
#else
	changeDirectory(SDL_GetBasePath());
#endif

	if (framework.init(GFX_SX, GFX_SY))
	{
		// initialize audio related systems
		
		SDL_mutex * mutex = SDL_CreateMutex();
		Assert(mutex != nullptr);

		AudioVoiceManagerBasic voiceMgr;
		voiceMgr.init(mutex, CHANNEL_COUNT, CHANNEL_COUNT);
		voiceMgr.outputStereo = true;

		AudioGraphManager_RTE audioGraphMgr(GFX_SX, GFX_SY);
		audioGraphMgr.init(mutex, &voiceMgr);

		AudioUpdateHandler audioUpdateHandler;
		audioUpdateHandler.init(mutex, nullptr, 0);
		audioUpdateHandler.voiceMgr = &voiceMgr;
		audioUpdateHandler.audioGraphMgr = &audioGraphMgr;

		PortAudioObject pa;
		pa.init(SAMPLE_RATE, 2, 2, AUDIO_UPDATE_SIZE, &audioUpdateHandler);
		
		// create an audio graph instance
		
		AudioGraphInstance * instance = audioGraphMgr.createInstance("sweetStuff6.xml");
		audioGraphMgr.selectInstance(instance);

		//

		Camera3d camera;
		camera.gamepadIndex = 0;
		camera.position = Vec3(0.f, 1.5f, -4.f);
		camera.pitch = -15.f;
		
		Surface surface(GFX_SX, GFX_SY, true, false, SURFACE_RGBA8);
		
		bool showDefaultEditor = false;
		
		while (!framework.quitRequested)
		{
			framework.process();

			const float dt = framework.timeStep;

			if (keyboard.wentDown(SDLK_ESCAPE))
				framework.quitRequested = true;
			
			if (keyboard.wentDown(SDLK_TAB))
				showDefaultEditor = !showDefaultEditor;
			
			camera.tick(dt, true);

			audioGraphMgr.tickEditor(dt, showDefaultEditor == false);
			
			framework.beginDraw(220, 220, 220, 0);
			{
				setFont("calibri.ttf");
				pushFontMode(FONT_SDF);
				{
					pushSurface(&surface);
					surface.clear();
					surface.clearDepth(1.f);
					
					const float fov = 60.f;
					const float near = .01f;
					const float far = 100.f;
					projectPerspective3d(fov, near, far);

					camera.pushViewMatrix();
					{
						glEnable(GL_DEPTH_TEST);
						glDepthFunc(GL_LEQUAL);
						
						setColor(100, 100, 100);
						drawGrid3dLine(10, 10, 0, 2, true);
						
						if (audioGraphMgr.selectedFile && instance)
						{
							drawEditor(*audioGraphMgr.selectedFile->graphEdit, instance->realTimeConnection, instance->audioGraph);
						}
						
						glDisable(GL_DEPTH_TEST);
					}
					camera.popViewMatrix();

					popSurface();
					
					projectScreen2d();
					
					float samples[AUDIO_UPDATE_SIZE * 2];
					voiceMgr.generateAudio(samples, AUDIO_UPDATE_SIZE);
					float magnitudeSq = 0.f;
					for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
						magnitudeSq += samples[i] * samples[i];
					magnitudeSq /= AUDIO_UPDATE_SIZE;
					const float blurStrength = magnitudeSq * 2000.f;
					setShader_GaussianBlurV(surface.getTexture(), 30, blurStrength);
					surface.postprocess();
					pushBlend(BLEND_OPAQUE);
					drawRect(0, 0, GFX_SX, GFX_SY);
					popBlend();
					clearShader();
					
					if (showDefaultEditor)
						audioGraphMgr.drawEditor();
					
					// show CPU usage of the audio thread
					
					setColor(255, 255, 255, 200);
					drawText(GFX_SX - 10, GFX_SY - 10, 16, -1, -1, "CPU usage audio thread: %d%%",
						int(std::round(audioUpdateHandler.msecsPerSecond / 1000000.0 * 100.0)));
				}
				popFontMode();
			}
			framework.endDraw();
		}
		
		// free the audio graph instance
		
		audioGraphMgr.free(instance, false);
		
		// shut down audio related systems

		pa.shut();
		
		audioUpdateHandler.shut();

		audioGraphMgr.shut();
		
		voiceMgr.shut();

		SDL_DestroyMutex(mutex);
		mutex = nullptr;

		framework.shutdown();
	}

	return 0;
}

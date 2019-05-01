#include "gfx-framework.h"
#include "jsusfx_file.h"
#include "jsusfx-framework.h"
#include "ui.h" // drawUiRectCheckered

#include "fileEditor_jsfx.h"
#include "imgui.h"
#include <algorithm>

void doMidiKeyboard(MidiKeyboard & kb, const int mouseX, const int mouseY, MidiBuffer & midiBuffer, const bool doTick, const bool doDraw, int & out_sx, int & out_sy, bool & inputIsCaptured)
{
	const int keySx = 16;
	const int keySy = 64;
	const int octaveSx = 24;
	const int octaveSy = 24;

	const int hoverIndex = mouseY >= 0 && mouseY <= keySy ? mouseX / keySx : -1;
	const float velocity = clamp(mouseY / float(keySy), 0.f, 1.f);

	const int octaveX = keySx * MidiKeyboard::kNumKeys + 4;
	const int octaveY = 4;
	const int octaveHoverIndex = mouseX >= octaveX && mouseX <= octaveX + octaveSx ? (mouseY - octaveY) / octaveSy : -1;

	const int sx = std::max(keySx * MidiKeyboard::kNumKeys, octaveX + octaveSx);
	const int sy = std::max(keySy, octaveSy * 2);

	out_sx = sx;
	out_sy = sy;

	if (doTick)
	{
		bool isDown[MidiKeyboard::kNumKeys];
		memset(isDown, 0, sizeof(isDown));

		if (inputIsCaptured == false)
		{
			if (mouse.isDown(BUTTON_LEFT) && hoverIndex >= 0 && hoverIndex < MidiKeyboard::kNumKeys)
				isDown[hoverIndex] = true;
		}

		for (int i = 0; i < MidiKeyboard::kNumKeys; ++i)
		{
			auto & key = kb.keys[i];

			if (key.isDown != isDown[i])
			{
				if (isDown[i])
				{
					key.isDown = true;

					const uint8_t message[3] = { MIDI_ON, (uint8_t)kb.getNote(i), uint8_t(velocity * 127) };
					
					midiBuffer.append(message, 3);
				}
				else
				{
					const uint8_t message[3] = { MIDI_OFF, (uint8_t)kb.getNote(i), uint8_t(velocity * 127) };

					midiBuffer.append(message, 3);

					key.isDown = false;
				}
			}
		}

		if (inputIsCaptured == false && mouse.wentDown(BUTTON_LEFT))
		{
			if (octaveHoverIndex == 0)
				kb.changeOctave(+1);
			if (octaveHoverIndex == 1)
				kb.changeOctave(-1);
		}

		for (int i = 0; i < MidiKeyboard::kNumKeys; ++i)
			inputIsCaptured |= kb.keys[i].isDown;
	}

	if (doDraw)
	{
		setColor(colorBlack);
		drawRect(0, 0, sx, sy);

		const Color colorKey(200, 200, 200);
		const Color colorkeyHover(255, 255, 255);
		const Color colorKeyDown(100, 100, 100);

		hqSetGradient(GRADIENT_LINEAR, Mat4x4(true).RotateZ(float(M_PI)/2.f).Scale(1.f, 1.f / keySy, 1.f), Color(140, 180, 220), colorWhite, COLOR_MUL);

		for (int i = 0; i < MidiKeyboard::kNumKeys; ++i)
		{
			auto & key = kb.keys[i];

			gxPushMatrix();
			{
				gxTranslatef(keySx * i, 0, 0);

				setColor(key.isDown ? colorKeyDown : i == hoverIndex ? colorkeyHover : colorKey);

				hqBegin(HQ_FILLED_RECTS);
				hqFillRect(0, 0, keySx, keySy);
				hqEnd();
			}
			gxPopMatrix();
		}

		hqClearGradient();

		{
			gxPushMatrix();
			{
				gxTranslatef(octaveX, octaveY, 0);

				//setLumi(200);
				//drawText(octaveSx + 4, 4, 12, +1, +1, "octave: %d", kb.octave);

				setColor(0 == octaveHoverIndex ? colorkeyHover : colorKey);
				hqSetGradient(GRADIENT_LINEAR, Mat4x4(true).RotateZ(float(M_PI)/2.f).Scale(1.f, 1.f / (octaveSy * 2), 1.f).Translate(-octaveX, -octaveY, 0), Color(140, 180, 220), colorWhite, COLOR_MUL);
				hqBegin(HQ_FILLED_ROUNDED_RECTS);
				hqFillRoundedRect(0, 0, octaveSx, octaveSy, 2.f);
				hqEnd();
				hqClearGradient();

				setLumi(40);
				hqBegin(HQ_FILLED_TRIANGLES);
				hqFillTriangle(octaveSx/2, 6, octaveSx - 6, octaveSy - 6, 6, octaveSy - 6);
				hqEnd();

				gxTranslatef(0, octaveSy, 0);

				setColor(1 == octaveHoverIndex ? colorkeyHover : colorKey);
				hqSetGradient(GRADIENT_LINEAR, Mat4x4(true).RotateZ(float(M_PI)/2.f).Scale(1.f, 1.f / (octaveSy * 2), 1.f).Translate(-octaveX, -octaveY, 0), Color(140, 180, 220), colorWhite, COLOR_MUL);
				hqBegin(HQ_FILLED_ROUNDED_RECTS);
				hqFillRoundedRect(0, 0, octaveSx, octaveSy, 2.f);
				hqEnd();
				hqClearGradient();

				setLumi(40);
				hqBegin(HQ_FILLED_TRIANGLES);
				hqFillTriangle(octaveSx/2, octaveSy - 6, octaveSx - 6, 6, 6, 6);
				hqEnd();
			}
			gxPopMatrix();
		}
		hqClearGradient();
	}
}

FileEditor_JsusFx::FileEditor_JsusFx(const char * path)
	: jsusFx(pathLibary)
	, gfx(jsusFx)
	, pathLibary(".")
{
	jsusFx.init();

	fileApi.init(jsusFx.m_vm);
	jsusFx.fileAPI = &fileApi;

	gfx.init(jsusFx.m_vm);
	jsusFx.gfx = &gfx;

	isValid = jsusFx.compile(pathLibary, path, JsusFx::kCompileFlag_CompileGraphicsSection);

	if (isValid)
	{
		mutex = SDL_CreateMutex();

		jsusFx.prepare(44100, 256);
		
		jsusFx.gfx_w = std::max(jsusFx.gfx_w, 440);
		jsusFx.gfx_h = std::max(jsusFx.gfx_h, 240);
		
		paObject.init(44100, 2, 0, 256, this);
	}
}

FileEditor_JsusFx::~FileEditor_JsusFx()
{
	paObject.shut();

	if (mutex != nullptr)
	{
		SDL_DestroyMutex(mutex);
		mutex = nullptr;
	}

	delete surface;
	surface = nullptr;
}

void FileEditor_JsusFx::portAudioCallback(
	const void * inputBuffer,
	const int numInputChannels,
	void * outputBuffer,
	const int numOutputChannels,
	const int framesPerBuffer)
{
	Assert(framesPerBuffer == 256);
	
	float inputSamples[2][256];
	
	if (false)
		memset(inputSamples, 0, sizeof(inputSamples));
	else
	{
		const float s = .1f;

		for (int i = 0; i < 256; ++i)
		{
			inputSamples[0][i] = random(-1.f, +1.f) * s;
			inputSamples[1][i] = random(-1.f, +1.f) * s;
		}
	}
	
	float outputSamples[2][256];
	
	const float * inputs[2] = { inputSamples[0], inputSamples[1] };
	float * outputs[2] = { outputSamples[0], outputSamples[1] };
	
	MidiBuffer midiBufferCopy;

	SDL_LockMutex(mutex);
	{
		midiBufferCopy = midiBuffer;

		midiBuffer.numBytes = 0;
	}
	SDL_UnlockMutex(mutex);

	jsusFx.setMidi(midiBufferCopy.bytes, midiBufferCopy.numBytes);
	
	jsusFx.process(inputs, outputs, framesPerBuffer, 2, 2);
	
	float * outputBufferf = (float*)outputBuffer;
	
	for (int i = 0; i < 256; ++i)
	{
		outputBufferf[i * 2 + 0] = outputSamples[0][i];
		outputBufferf[i * 2 + 1] = outputSamples[1][i];
	}
}

void FileEditor_JsusFx::doButtonBar()
{
	ImGui::Checkbox("Midi Keyboard", &showMidiKeyboard);
	
	ImGui::SameLine();
	ImGui::Checkbox("Control sliders", &showControlSliders);
}

void FileEditor_JsusFx::tick(const int sx, const int sy, const float dt, const bool hasFocus, bool & inputIsCaptured)
{
	if (surface == nullptr || surface->getWidth() != sx || surface->getHeight() != sy)
	{
		delete surface;
		surface = nullptr;

		surface = new Surface(sx, sy, false);
	}
	
	clearSurface(0, 0, 0, 0);

	pushSurface(surface);
	{
		setColor(colorWhite);
		drawUiRectCheckered(0, 0, sx, sy, 8);
	
		int midiSx;
		int midiSy;

		if (showMidiKeyboard)
		{
			doMidiKeyboard(midiKeyboard, mouse.x, mouse.y, midiBuffer, true, false, midiSx, midiSy, inputIsCaptured);
		}
		
		if (isValid == false)
		{
			setFont("calibri.ttf");
			pushFontMode(FONT_SDF);
			setColor(colorWhite);
			drawText(sx/2, sy/2, 16, 0, 0, "Failed to load jsfx file");
			popFontMode();
		}
		else
		{
			if (jsusFx.hasGraphicsSection())
			{
				setFont("calibri.ttf");
				pushFontMode(FONT_SDF);
				setColorClamp(true);
				{
					const int gfxSx = jsusFx.gfx_w;
					const int gfxSy = jsusFx.gfx_h;

					if (firstFrame)
					{
						firstFrame = false;

						offsetX = (sx - gfxSx) / 2;
						offsetY = (sy - gfxSy) / 2;
					}

					if (inputIsCaptured == false)
					{
						const bool isOutside =
							mouse.x < offsetX || mouse.x > offsetX + jsusFx.gfx_w ||
							mouse.y < offsetY || mouse.y > offsetY + jsusFx.gfx_h;

						if (isOutside && mouse.wentDown(BUTTON_LEFT))
						{
							isDragging = true;
						}

						if (mouse.wentUp(BUTTON_LEFT))
						{
							isDragging = false;
						}
					}
					else
					{
						isDragging = false;
					}
				
					if (isDragging)
					{
						inputIsCaptured = true;

						offsetX = mouse.x - gfxSx/2;
						offsetY = mouse.y;
					}
				
					if (inputIsCaptured == false)
					{
						if (keyboard.wentDown(SDLK_f) && keyboard.isDown(SDLK_LSHIFT))
						{
							inputIsCaptured = true;
						
							jsusFx.gfx_w = sx;
							jsusFx.gfx_h = sy;
						
							offsetX = 0;
							offsetY = 0;
						}
					}

					mouse.x -= offsetX;
					mouse.y -= offsetY;
					{
						setDrawRect(offsetX, offsetY, gfxSx, gfxSy);
						gfx.drawTransform.MakeTranslation(offsetX, offsetY, 0.f);

						gfx.setup(surface, gfxSx, gfxSy, mouse.x, mouse.y, inputIsCaptured == false);

						jsusFx.draw();

						clearDrawRect();
					}
					mouse.x += offsetX;
					mouse.y += offsetY;
				}
				setColorClamp(false);
				popFontMode();
			}
		
			if (showControlSliders)
			{
				setFont("calibri.ttf");
				pushFontMode(FONT_SDF);
				{
					const int margin = 10;

					int numSliders = 0;
				
					for (auto & slider : jsusFx.sliders)
						if (slider.exists && slider.desc[0] != '-')
							numSliders++;
				
					const int sx = 200;
					const int sy = 20;
					const int advanceY = 22;
					
					const int totalSx = sx + margin * 2;
					const int totalSy = advanceY * numSliders + margin * 2;
					
					if (firstFrame)
					{
						firstFrame = false;
						
						offsetX = (surface->getWidth() - totalSx) / 2;
						offsetY = (surface->getWidth() - totalSy) / 2;
					}
					
					int x = offsetX;
					int y = offsetY;
					
					int sliderIndex = 0;
					
					auto doSlider = [&](JsusFx & fx, JsusFx_Slider & slider, int x, int y, bool & isActive)
						{
							const bool isInside =
								x >= 0 && x <= sx &&
								y >= 0 && y <= sy;
						
							if (isInside && mouse.wentDown(BUTTON_LEFT))
								isActive = true;
						
							if (mouse.wentUp(BUTTON_LEFT))
								isActive = false;
						
							if (isActive)
							{
								const float t = x / float(sx);
								const float v = slider.min + (slider.max - slider.min) * t;
								fx.moveSlider(&slider - fx.sliders, v);
							}
						
							setColor(0, 0, 255, 127);
							const float t = (slider.getValue() - slider.min) / (slider.max - slider.min);
							drawRect(0, 0, sx * t, sy);
						
							if (slider.isEnum)
							{
								const int enumIndex = (int)slider.getValue();
							
								if (enumIndex >= 0 && enumIndex < slider.enumNames.size())
								{
									setColor(colorWhite);
									drawText(sx/2.f, sy/2.f, 14.f, 0.f, 0.f, "%s", slider.enumNames[enumIndex].c_str());
								}
							}
							else
							{
								setColor(colorWhite);
								drawText(sx/2.f, sy/2.f, 14.f, 0.f, 0.f, "%s", slider.desc);
							}
						
							setColor(63, 31, 255, 127);
							drawRectLine(0, 0, sx, sy);
						};

					if (numSliders > 0)
					{
						setColor(0, 0, 0, 127);
						drawRect(x, y, x + totalSx, y + totalSy);
					
						x += margin;
						y += margin;
						
						for (auto & slider : jsusFx.sliders)
						{
							if (slider.exists && slider.desc[0] != '-')
							{
								gxPushMatrix();
								gxTranslatef(x, y, 0);
								doSlider(jsusFx, slider, mouse.x - x, mouse.y - y, sliderIsActive[sliderIndex]);
								gxPopMatrix();

								y += advanceY;
							}

							sliderIndex++;
						}
					}
				}
				popFontMode();
			}
		}

		if (showMidiKeyboard)
		{
			doMidiKeyboard(midiKeyboard, mouse.x, mouse.y, midiBuffer, false, true, midiSx, midiSy, inputIsCaptured);
		}
	}
	popSurface();

	surface->blit(BLEND_OPAQUE);
}

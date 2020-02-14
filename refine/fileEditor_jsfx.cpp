#include "gfx-framework.h"
#include "jsusfx_file.h"
#include "jsusfx-framework.h"
#include "reflection.h"
#include "ui.h" // drawUiRectCheckered

#include "fileEditor_jsfx.h"
#include "imgui.h"
#include <algorithm>

#define ENABLE_MIDI 1

#if ENABLE_MIDI
	#include "rtmidi/RtMidi.h"
#endif

#define MIDI_OFF 0x80
#define MIDI_ON 0x90

static const bool kShowHiddenSliders = true;

void doMidiKeyboard(
	MidiKeyboard & kb,
	const int mouseX,
	const int mouseY,
	MidiBuffer & midiBuffer,
	const bool doTick,
	const bool doDraw,
	const bool isFocused,
	const int sx,
	const int sy,
	bool & inputIsCaptured)
{
	const int kOctavePadding = 4;
	
	const int octaveSx = 24;
	const int octaveSy = std::min<int>(sy/2, 24);

	const int spaceForKeys = sx - octaveSx - kOctavePadding;
	const int keySx = spaceForKeys / MidiKeyboard::kNumKeys;
	const int keySy = sy;

	const int hoverIndex = inputIsCaptured ? -1 : (mouseY >= 0 && mouseY <= keySy ? mouseX / keySx : -1);
	float velocity = clamp(mouseY / float(keySy), 0.f, 1.f);

	const int octaveX = keySx * MidiKeyboard::kNumKeys + 4;
	const int octaveY = 0;
	const int octaveHoverIndex = mouseX >= octaveX && mouseX <= octaveX + octaveSx ? (mouseY - octaveY) / octaveSy : -1;

	if (doTick)
	{
		bool isDown[MidiKeyboard::kNumKeys];
		memset(isDown, 0, sizeof(isDown));

		if (inputIsCaptured == false)
		{
			if (mouse.isDown(BUTTON_LEFT) && hoverIndex >= 0 && hoverIndex < MidiKeyboard::kNumKeys)
				isDown[hoverIndex] = true;
			
			if (isFocused)
			{
				for (int i = 1; i < 10 && i < MidiKeyboard::kNumKeys; ++i)
				{
					if (keyboard.isDown(SDLK_0 + i))
					{
						isDown[i] = true;
						velocity = 1.f;
					}
				}
			}
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

//

void JsusFxWindow::init(const int x, const int y, const int clientSx, const int clientSy, const char * caption)
{
	this->x = x;
	this->y = y;
	this->clientSx = clientSx;
	this->clientSy = clientSy;
	this->caption = caption;
}

void JsusFxWindow::tick(const float dt, bool & inputIsCaptured)
{
	if (inputIsCaptured || isVisible == false)
	{
		state = kState_Idle;
		isFocused = false;
	}
	else
	{
		const int sx = kBorderSize + clientSx + kBorderSize;
		const int sy = kBorderSize + kTitlebarSize + clientSy + kBorderSize;

		const bool isInside =
			mouse.x >= x && mouse.x < x + sx &&
			mouse.y >= y && mouse.y < y + sy;
		
		const bool isInsideTitleBar =
			mouse.x >= x + kBorderSize && mouse.x < x + sx - kBorderSize &&
			mouse.y >= y + kBorderSize && mouse.y < y + kBorderSize + kTitlebarSize;

		if (state == kState_Idle)
		{
			if (mouse.wentDown(BUTTON_LEFT))
			{
				if (isInside)
					isFocused = true;
				else
					isFocused = false;
				
				if (isInsideTitleBar)
				{
					state = kState_DragMove;
					inputIsCaptured = true;
				}
			}
		}
		else if (state == kState_DragMove)
		{
			inputIsCaptured = true;
			
			if (mouse.wentUp(BUTTON_LEFT))
			{
				state = kState_Idle;
			}
			else
			{
				x += mouse.dx;
				y += mouse.dy;
			}
		}
	}
}

void JsusFxWindow::drawDecoration() const
{
	if (isVisible == false)
	{
		return;
	}
	
	gxPushMatrix();
	{
		gxTranslatef(x, y, 0);

		const int sx = kBorderSize + clientSx + kBorderSize;
		const int sy = kBorderSize + kTitlebarSize + clientSy + kBorderSize;

		// draw borders

		setColor(colorBlack);
		drawRect(0, 0, sx, sy);
		drawRect(kBorderSize, kBorderSize, sx - kBorderSize, sy - kBorderSize);

		// draw title bar area

		setAlpha(255);
		setLumi(isFocused ? 60 : 40);
		drawRect(kBorderSize, kBorderSize, sx - kBorderSize, kBorderSize + kTitlebarSize);
		
		if (caption != nullptr)
		{
			setFont("calibri.ttf");
			pushFontMode(FONT_SDF);
			setColor(140, 140, 100);
			drawText(sx/2, kBorderSize + kTitlebarSize/2, 16, 0, 0, "%s", caption);
			popFontMode();
		}
	}
	gxPopMatrix();
}

void JsusFxWindow::getClientRect(int & x, int & y, int & sx, int & sy) const
{
	x = this->x + kBorderSize;
	y = this->y + kBorderSize + kTitlebarSize;;

	sx = clientSx;
	sy = clientSy;
}

//

FileEditor_JsusFx::FileEditor_JsusFx(const char * path)
	: jsusFx(pathLibary)
	, gfx(jsusFx)
	, pathLibary(".")
{
// todo : add option to path library to search all sub folders
	pathLibary.addSearchPath("lib"); // for Kawa scripts
	
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
		
		paObject.init(44100, 2, 1, 256, this);
		
		jsusFxWindow.init(10, 100, jsusFx.gfx_w, jsusFx.gfx_h, jsusFx.desc);
	}
	else
	{
		jsusFxWindow.init(10, 100, 200, 100, nullptr);
	}

	midiKeyboardWindow.init(10, 10, 300, 40, "Midi Keyboard");
	
	controlSlidersWindow.init(20, 20, 300, 400, "Sliders");
	controlSlidersWindow.isVisible = false;

	try
	{
		midiIn = new RtMidiIn(RtMidi::UNSPECIFIED, "Midi Controller", 1024);
	}
	catch (std::exception & e)
	{
		logError("failed to create MIDI input: %s", e.what());
	}
}

FileEditor_JsusFx::~FileEditor_JsusFx()
{
#if ENABLE_MIDI
	if (midiIn != nullptr)
	{
		if (midiIn->isPortOpen())
			midiIn->closePort();
		
		delete midiIn;
		midiIn = nullptr;
	}
#endif

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
	
	const AudioSource audioSource = synthesis.audioSource.load();
	const float volume = synthesis.volume.load() / 100.f;
	const float frequency = synthesis.frequency.load();
	const float sharpness = synthesis.sharpness.load() / 100.f;
	
	if (audioSource == kAudioSource_Silence)
	{
		memset(inputSamples, 0, sizeof(inputSamples));
	}
	else if (audioSource == kAudioSource_PinkNoise)
	{
		const float scale1 = 1.f / (1 << 16);
		const float scale2 = 2.f * volume;
		
		for (int i = 0; i < framesPerBuffer; ++i)
		{
			const float value = (synthesis.pinkNumber.next() * scale1 - .5f) * scale2;
			
			inputSamples[0][i] = value;
			inputSamples[1][i] = value;
		}
	}
	else if (audioSource == kAudioSource_WhiteNoise)
	{
		for (int i = 0; i < framesPerBuffer; ++i)
		{
			const float value = random(-1.f, +1.f) * volume;
			
			inputSamples[0][i] = value;
			inputSamples[1][i] = value;
		}
	}
	else if (audioSource == kAudioSource_Sine)
	{
		const float twoPi = 2.f * float(M_PI);
		const float phaseStep = frequency * twoPi / 44100.f;
		
		synthesis.sinePhase = fmodf(synthesis.sinePhase, twoPi);
		
		for (int i = 0; i < framesPerBuffer; ++i)
		{
			const float value = sinf(synthesis.sinePhase) * volume;
			
			inputSamples[0][i] = value;
			inputSamples[1][i] = value;
			
			synthesis.sinePhase += phaseStep;
		}
	}
	else if (audioSource == kAudioSource_Tent)
	{
		memset(inputSamples, 0, sizeof(inputSamples));
		
		const float phaseStep = frequency / 44100.f;
		
		const float scale = 1.f / (1.f - sharpness + 1e-6f);
		
		for (int i = 0; i < framesPerBuffer; ++i)
		{
			// 0.0 --> 0.5 --> 0.0
			float value = synthesis.tentPhase < .5f ? synthesis.tentPhase : 1.f - synthesis.tentPhase;
			
			// -1.0 --> +1.0 --> -1.0
			value = value * 4.f - 1.f;
			
			// sharpness : -SCALE --> +SCALE --> -SCALE
			value *= scale;
			
			// clamp : -1.0 --> +1.0 --> -1.0 (with sharpness applied)
			value = value < -1.f ? -1.f : value > +1.f ? +1.f : value;
			
			value *= volume;
			
			inputSamples[0][i] = value;
			inputSamples[1][i] = value;
			
			synthesis.tentPhase += phaseStep;
			
			synthesis.tentPhase = fmodf(synthesis.tentPhase, 1.f);
		}
	}
	else if (audioSource == kAudioSource_AudioInterface)
	{
		float * input = (float*)inputBuffer;
		
		for (int i = 0; i < framesPerBuffer; ++i)
		{
			inputSamples[0][i] = numInputChannels >= 1 ? input[i * numInputChannels + 0] : 0.f;
			inputSamples[1][i] = numInputChannels >= 2 ? input[i * numInputChannels + 1] : inputSamples[0][i];
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
	ImGui::Checkbox("Midi Keyboard", &midiKeyboardWindow.isVisible);
	
	ImGui::SameLine();
	ImGui::Checkbox("Control sliders", &controlSlidersWindow.isVisible);
	
	const char * items[] =
	{
		"Silence",
		"Pink noise",
		"White noise",
		"Sine wave",
		"Tent wave",
		"Audio Interface"
	};
	
	if (ImGui::BeginMenu("Audio source"))
	{
		int audioSource = this->audioSource;
		if (ImGui::Combo("Type", &audioSource, items, sizeof(items) / sizeof(items[0])))
		{
			this->audioSource = (AudioSource)audioSource;
			updateSynthesisParams();
		}
		
		if (ImGui::SliderInt("Volume", &volume, 0, 100))
			updateSynthesisParams();
		
		if (ImGui::SliderInt("Hz", &frequency, 0, 4000))
			updateSynthesisParams();
		
		if (ImGui::SliderInt("Sharpness", &sharpness, 0, 100))
			updateSynthesisParams();
		
		ImGui::EndMenu();
	}
}

void FileEditor_JsusFx::updateMidi()
{
#if ENABLE_MIDI
	if (midiIn == nullptr)
		return;
	
	// open the desired midi port

	if (currentMidiPort != desiredMidiPort)
	{
		currentMidiPort = desiredMidiPort;
		
		for (int i = 0; i < midiIn->getPortCount(); ++i)
		{
			auto name = midiIn->getPortName();
			
			logDebug("available MIDI port: %d: %s", i, name.c_str());
		}

		if (desiredMidiPort < midiIn->getPortCount())
		{
			midiIn->openPort(desiredMidiPort);
			
			if (midiIn->isPortOpen() == false)
			{
				logWarning("failed to open desired midi port %d", desiredMidiPort);
			}
		}
	}

	// receive midi messages

	std::vector<uint8_t> messageBytes;

	for (;;)
	{
		midiIn->getMessage(&messageBytes);
		
		if (messageBytes.empty())
			break;
		
		if (midiBuffer.append(&messageBytes[0], messageBytes.size()) == false)
			break;
	}
#endif
}

void FileEditor_JsusFx::updateSynthesisParams()
{
	synthesis.audioSource.store(audioSource);
	synthesis.volume.store(volume);
	synthesis.frequency.store(frequency);
	synthesis.sharpness.store(sharpness);
}

bool FileEditor_JsusFx::reflect(TypeDB & typeDB, StructuredType & type)
{
	typeDB.addEnum<FileEditor_JsusFx::AudioSource>("FileEditor_JsusFx::AudioSource")
		.add("silence", kAudioSource_Silence)
		.add("pinkNoise", kAudioSource_PinkNoise)
		.add("whiteNoise", kAudioSource_WhiteNoise)
		.add("sine", kAudioSource_Sine)
		.add("tent", kAudioSource_Tent)
		.add("audioInterface", kAudioSource_AudioInterface)
		.add("sample", kAudioSource_Sample);
	
	type.add("audioSource", &FileEditor_JsusFx::audioSource);
	type.add("volume", &FileEditor_JsusFx::volume);
	type.add("frequency", &FileEditor_JsusFx::frequency);
	type.add("sharpness", &FileEditor_JsusFx::sharpness);
	
	return true;
}

void FileEditor_JsusFx::tick(const int sx, const int sy, const float dt, const bool hasFocus, bool & inputIsCaptured)
{
	updateSynthesisParams();
	
	//
	
	clearSurface(0, 0, 0, 0);

	setColor(colorWhite);
	drawUiRectCheckered(0, 0, sx, sy, 8);
	
	controlSlidersWindow.tick(dt, inputIsCaptured);
	
	midiKeyboardWindow.tick(dt, inputIsCaptured);
	
	jsusFxWindow.tick(dt, inputIsCaptured);

	if (midiKeyboardWindow.isVisible)
	{
		int x, y;
		int sx, sy;
		midiKeyboardWindow.getClientRect(x, y, sx, sy);
		
		doMidiKeyboard(
			midiKeyboard,
			mouse.x - x,
			mouse.y - y,
			midiBuffer,
			true,
			false,
			midiKeyboardWindow.isFocused,
			sx,
			sy,
			inputIsCaptured);
	}

	updateMidi();

	//
	
	jsusFxWindow.drawDecoration();
	
	{
		int x, y;
		int sx, sy;
		jsusFxWindow.getClientRect(x, y, sx, sy);
		
		if (isValid == false)
		{
			setFont("calibri.ttf");
			pushFontMode(FONT_SDF);
			setColor(colorWhite);
			drawText(x + sx/2, x + sy/2, 16, 0, 0, "Failed to load jsfx file");
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
						if (keyboard.wentDown(SDLK_f) && keyboard.isDown(SDLK_LSHIFT))
						{
							inputIsCaptured = true;
						
							jsusFx.gfx_w = sx;
							jsusFx.gfx_h = sy;
						
							offsetX = 0;
							offsetY = 0;
						}
					}
					
					if (surface == nullptr ||
						surface->getWidth() != gfxSx ||
						surface->getHeight() != gfxSy)
					{
						delete surface;
						surface = nullptr;

						surface = new Surface(gfxSx, gfxSy, false);
					}

					mouse.x -= x + offsetX;
					mouse.y -= y + offsetY;
					{
						gfx.setup(surface, gfxSx, gfxSy, mouse.x, mouse.y, inputIsCaptured == false);

						jsusFx.draw();
					}
					mouse.x += x + offsetX;
					mouse.y += y + offsetY;
				}
				setColorClamp(false);
				popFontMode();
				
				pushBlend(BLEND_OPAQUE);
				gxSetTexture(surface->getTexture());
				setColor(colorWhite);
				drawRect(
					x + offsetX,
					y + offsetY,
					x + offsetX + surface->getWidth(),
					y + offsetY + surface->getHeight());
				gxSetTexture(0);
				popBlend();
			}
		}
	}

	if (midiKeyboardWindow.isVisible)
	{
		midiKeyboardWindow.drawDecoration();
	
		int x, y;
		int sx, sy;
		midiKeyboardWindow.getClientRect(x, y, sx, sy);
		
		gxPushMatrix();
		{
			gxTranslatef(x, y, 0);
		
			doMidiKeyboard(
				midiKeyboard,
				mouse.x - x,
				mouse.y - y,
				midiBuffer,
				false,
				true,
				midiKeyboardWindow.isFocused,
				sx,
				sy,
				inputIsCaptured);
		}
		gxPopMatrix();
	}
	
	if (controlSlidersWindow.isVisible)
	{
		controlSlidersWindow.drawDecoration();
		
		gxPushMatrix();
		{
			int wx, wy;
			int sx, sy;
			controlSlidersWindow.getClientRect(wx, wy, sx, sy);
		
			gxTranslatef(wx, wy, 0);
			
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
				setFont("calibri.ttf");
				pushFontMode(FONT_SDF);
				{
					const int margin = 10;

					int numSliders = 0;
				
					for (auto & slider : jsusFx.sliders)
						if (slider.exists && (slider.desc[0] != '-' || kShowHiddenSliders))
							numSliders++;
				
					const int slider_sx = sx - margin * 2;
					const int slider_sy = 20;
					const int slider_advanceY = 22;
					
					const int totalSx = slider_sx + margin * 2;
					const int totalSy = slider_advanceY * numSliders + margin * 2;
					
					int x = 0;
					int y = 0;
					
					int sliderIndex = 0;
					
					auto doSlider = [&](JsusFx & fx, JsusFx_Slider & slider, int x, int y, bool & isActive)
						{
							const bool isInside =
								x >= 0 && x <= slider_sx &&
								y >= 0 && y <= slider_sy;
						
							if (isInside && mouse.wentDown(BUTTON_LEFT))
								isActive = true;
						
							if (mouse.wentUp(BUTTON_LEFT))
								isActive = false;
						
							if (isActive)
							{
								const float t = x / float(slider_sx);
								const float v = slider.min + (slider.max - slider.min) * t;
								fx.moveSlider(&slider - fx.sliders, v);
							}
						
							setColor(0, 0, 255, 127);
							const float t = (slider.getValue() - slider.min) / (slider.max - slider.min);
							drawRect(0, 0, slider_sx * t, slider_sy);
						
							if (slider.isEnum)
							{
								const int enumIndex = (int)slider.getValue();
							
								if (enumIndex >= 0 && enumIndex < slider.enumNames.size())
								{
									setColor(colorWhite);
									drawText(slider_sx/2.f, slider_sy/2.f, 14.f, 0.f, 0.f, "%s", slider.enumNames[enumIndex].c_str());
								}
							}
							else
							{
								setColor(colorWhite);
								drawText(slider_sx/2.f, slider_sy/2.f, 14.f, 0.f, 0.f, "%s", slider.desc);
							}
						
							setColor(63, 31, 255, 127);
							drawRectLine(0, 0, slider_sx, slider_sy);
						};

					if (numSliders > 0)
					{
						setColor(0, 0, 0, 127);
						drawRect(x, y, x + totalSx, y + totalSy);
					
						x += margin;
						y += margin;
						
						for (auto & slider : jsusFx.sliders)
						{
							if (slider.exists && (slider.desc[0] != '-' || kShowHiddenSliders))
							{
								gxPushMatrix();
								gxTranslatef(x, y, 0);
								doSlider(jsusFx, slider, mouse.x - x - wx, mouse.y - y - wy, sliderIsActive[sliderIndex]);
								gxPopMatrix();

								y += slider_advanceY;
							}

							sliderIndex++;
						}
					}
				}
				popFontMode();
			}
		}
		gxPopMatrix();
	}
}

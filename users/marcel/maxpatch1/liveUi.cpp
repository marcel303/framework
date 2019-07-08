#include "Debugging.h"
#include "framework.h"
#include "liveUi.h"
#include "osc/OscOutboundPacketStream.h"
#include "oscSender.h"
#include <math.h>

LiveUi & LiveUi::osc(const char * ipAddress, const int udpPort)
{
	OscSender * sender = new OscSender();
	sender->init(ipAddress, udpPort);
	oscSenders.push_back(sender);
	return *this;
}

void LiveUi::addElem(ControlSurfaceDefinition::Element * elem)
{
	elems.resize(elems.size() + 1);
	
	auto & e = elems.back();
	e.elem = elem;
	
	if (elem->type == ControlSurfaceDefinition::kElementType_Knob)
	{
		auto & knob = elem->knob;
		Assert(knob.hasDefaultValue);
		
		if (knob.hasDefaultValue)
		{
			const float t = (knob.defaultValue - knob.min) / (knob.max - knob.min);
			
			e.value = powf(t, 1.f / knob.exponential);
			e.defaultValue = e.value;
		}
	}
	if (elem->type == ControlSurfaceDefinition::kElementType_Slider3)
	{
		auto & slider = elem->slider3;
		Assert(slider.hasDefaultValue);
		
		if (slider.hasDefaultValue)
		{
			for (int i = 0; i < 3; ++i)
				e.value4[i] = slider.defaultValue[i];
		
		// todo : store default value
			//e.defaultValue = e.value;
		}
	}
	else if (elem->type == ControlSurfaceDefinition::kElementType_Listbox)
	{
		auto & listbox = elem->listbox;
		Assert(listbox.hasDefaultValue);
		
		if (listbox.hasDefaultValue)
		{
			int index = 0;
			
			for (int i = 0; i < listbox.items.size(); ++i)
				if (listbox.items[i] == listbox.defaultValue)
					index = i;
			
			e.value = index;
			e.defaultValue = index;
		}
	}
	else if (elem->type == ControlSurfaceDefinition::kElementType_ColorPicker)
	{
		auto & colorPicker = elem->colorPicker;
		Assert(colorPicker.hasDefaultValue);
		
		if (colorPicker.hasDefaultValue)
		{
			if (elem->colorPicker.colorSpace == ControlSurfaceDefinition::kColorSpace_Rgb)
			{
				const Color color(
					colorPicker.defaultValue.x,
					colorPicker.defaultValue.y,
					colorPicker.defaultValue.z);
				
				float hue, saturation, luminance;
				color.toHSL(hue, saturation, luminance);
				
				e.value4.x = hue;
				e.value4.y = saturation;
				e.value4.z = luminance;
			}
			else if (elem->colorPicker.colorSpace == ControlSurfaceDefinition::kColorSpace_Rgbw)
			{
				const Color color(
					colorPicker.defaultValue.x,
					colorPicker.defaultValue.y,
					colorPicker.defaultValue.z);
				
				float hue;
				float saturation;
				float luminance;
				color.toHSL(hue, saturation, luminance);
				
				e.value4.x = hue;
				e.value4.y = saturation;
				e.value4.z = luminance;
				e.value4.w = colorPicker.defaultValue.w;
			}
			else if (elem->colorPicker.colorSpace == ControlSurfaceDefinition::kColorSpace_Hsl)
			{
				e.value4 = colorPicker.defaultValue;
			}
		}
		else
		{
			e.value4.x = 0.f;
			e.value4.y = 1.f;
			e.value4.z = .5f;
		}
	}
}

void LiveUi::tick(const float dt)
{
	hoverElem = nullptr;
	
	for (auto & e : elems)
	{
		auto * elem = e.elem;
		
		e.doubleClickTimer = fmaxf(0.f, e.doubleClickTimer - dt);
		
		const bool isInside =
			mouse.x >= elem->x &&
			mouse.x < elem->x + elem->sx &&
			mouse.y >= elem->y &&
			mouse.y < elem->y + elem->sy;
		
		if (isInside)
			hoverElem = &e;
		
		if (elem->type == ControlSurfaceDefinition::kElementType_Knob)
		{
			auto & knob = elem->knob;
			
			if (activeElem == nullptr && isInside && mouse.wentDown(BUTTON_LEFT))
			{
				activeElem = &e;
				SDL_CaptureMouse(SDL_TRUE);
				
				if (e.doubleClickTimer > 0.f)
				{
					if (knob.hasDefaultValue)
						e.value = e.defaultValue;
				}
				else
					e.doubleClickTimer = .2f;
			}
			
			if (&e == activeElem && mouse.wentUp(BUTTON_LEFT))
			{
				activeElem = nullptr;
				SDL_CaptureMouse(SDL_FALSE);
			}
			
			if (&e == activeElem)
			{
				const float speed = 1.f / (keyboard.isDown(SDLK_LSHIFT) ? 400.f : 100.f);
				
				const float oldValue = e.value;
				
				e.value = saturate<float>(e.value - mouse.dy * speed);
				
				if (e.value != oldValue)
				{
					e.valueHasChanged = true;
				}
			}
		}
		if (elem->type == ControlSurfaceDefinition::kElementType_Slider3)
		{
			auto & slider = elem->slider3;
			
			if (activeElem == nullptr && isInside && mouse.wentDown(BUTTON_LEFT))
			{
				activeElem = &e;
				SDL_CaptureMouse(SDL_TRUE);
				
				e.liveState[0] = clamp<int>((mouse.x - elem->x) * 3 / elem->sx, 0, 2);
				
				if (e.doubleClickTimer > 0.f)
				{
				// fixme : assign value4
					if (slider.hasDefaultValue)
						e.value = e.defaultValue;
				}
				else
					e.doubleClickTimer = .2f;
			}
			
			if (&e == activeElem && mouse.wentUp(BUTTON_LEFT))
			{
				activeElem = nullptr;
				SDL_CaptureMouse(SDL_FALSE);
			}
			
			if (&e == activeElem)
			{
				const float speed = 1.f / (keyboard.isDown(SDLK_LSHIFT) ? 400.f : 100.f);
				
				const auto oldValue = e.value4;
				
				const int index = e.liveState[0];
				
				e.value4[index] = saturate<float>(e.value4[index] - mouse.dy * speed);
				
				if (e.value4 != oldValue)
				{
					e.valueHasChanged = true;
				}
			}
		}
		else if (elem->type == ControlSurfaceDefinition::kElementType_Listbox)
		{
			auto & listbox = elem->listbox;
			
			if (activeElem == nullptr && isInside && mouse.wentDown(BUTTON_LEFT))
			{
				activeElem = &e;
				SDL_CaptureMouse(SDL_TRUE);
				
				if (e.doubleClickTimer > 0.f)
				{
					if (listbox.hasDefaultValue)
						e.value = e.defaultValue;
				}
				else
					e.doubleClickTimer = .2f;
			}
			
			if (&e == activeElem && mouse.wentUp(BUTTON_LEFT))
			{
				activeElem = nullptr;
				SDL_CaptureMouse(SDL_FALSE);
			}
			
			if (&e == activeElem)
			{
				const float speed = 1.f / 100.f;
				
				const float oldValue = e.value;
				
				e.value = clamp<float>(e.value + mouse.dy * speed, 0.f, listbox.items.size());
				
				if (e.value != oldValue)
				{
				// todo: only changed when item index changes
					e.valueHasChanged = true;
				}
			}
		}
		else if (elem->type == ControlSurfaceDefinition::kElementType_ColorPicker)
		{
			auto & colorPicker = elem->colorPicker;
			
			if (colorPicker.colorSpace == ControlSurfaceDefinition::kColorSpace_Hsl ||
				colorPicker.colorSpace == ControlSurfaceDefinition::kColorSpace_Rgb)
			{
				const int picker_x = 0;
				const int picker_sx = elem->sx - 25;
				const int slider_x = elem->sx - 20;
				const int slider_sx = 20;
				
				if (activeElem == nullptr && isInside && mouse.wentDown(BUTTON_LEFT))
				{
					const bool isInsidePicker =
						mouse.x >= elem->x + picker_x &&
						mouse.x < elem->x + picker_x + picker_sx;
					
					const bool isInsideSlider =
						mouse.x >= elem->x + slider_x &&
						mouse.x < elem->x + slider_x + slider_sx;
					
					if (isInsidePicker)
					{
						activeElem = &e;
						SDL_CaptureMouse(SDL_TRUE);
						
						e.liveState[0] = 'p';
						
						if (e.doubleClickTimer > 0.f)
						{
							if (colorPicker.hasDefaultValue)
								e.value4 = colorPicker.defaultValue;
						}
						else
							e.doubleClickTimer = .2f;
					}
					else if (isInsideSlider)
					{
						activeElem = &e;
						SDL_CaptureMouse(SDL_TRUE);
						
						e.liveState[0] = '1';
						
						if (e.doubleClickTimer > 0.f)
						{
							if (colorPicker.hasDefaultValue)
								e.value4 = colorPicker.defaultValue;
						}
						else
							e.doubleClickTimer = .2f;
					}
				}
				
				if (&e == activeElem && mouse.wentUp(BUTTON_LEFT))
				{
					e.liveState[0] = 0;
					
					activeElem = nullptr;
					SDL_CaptureMouse(SDL_FALSE);
				}
			
				if (&e == activeElem)
				{
					ControlSurfaceDefinition::Vector4 oldValue = e.value4;
					
					if (e.liveState[0] == 'p')
					{
						const float hue = saturate<float>((mouse.x - elem->x - picker_x) / float(picker_sx));
						const float lightness = saturate<float>(1.f - (mouse.y - elem->y) / float(elem->sy));
						
						e.value4.x = hue;
						e.value4.z = lightness;
					}
					else if (e.liveState[0] == '1')
					{
						const float saturation = saturate<float>(1.f - (mouse.y - elem->y) / float(elem->sy));
						
						e.value4.y = saturation;
					}
					else if (e.liveState[0] == '2')
					{
						const float saturation = saturate<float>(1.f - (mouse.y - elem->y) / float(elem->sy));
						
						e.value4.y = saturation;
					}
					
					if (e.value4 != oldValue)
					{
						e.valueHasChanged = true;
					}
				}
			}
			else
			{
				const int picker_x = 0;
				const int picker_sx = elem->sx - 25;
				const int slider1_x = elem->sx - 20;
				const int slider1_sx = 10;
				const int slider2_x = elem->sx - 10;
				const int slider2_sx = 10;
				
				if (activeElem == nullptr && isInside && mouse.wentDown(BUTTON_LEFT))
				{
					const bool isInsidePicker =
						mouse.x >= elem->x + picker_x &&
						mouse.x < elem->x + picker_x + picker_sx;
					
					const bool isInsideSlider1 =
						mouse.x >= elem->x + slider1_x &&
						mouse.x < elem->x + slider1_x + slider1_sx;
					
					const bool isInsideSlider2 =
						mouse.x >= elem->x + slider2_x &&
						mouse.x < elem->x + slider2_x + slider2_sx;
					
					if (isInsidePicker)
					{
						activeElem = &e;
						SDL_CaptureMouse(SDL_TRUE);
						
						e.liveState[0] = 'p';
						
						if (e.doubleClickTimer > 0.f)
						{
							if (colorPicker.hasDefaultValue)
								e.value4 = colorPicker.defaultValue;
						}
						else
							e.doubleClickTimer = .2f;
					}
					else if (isInsideSlider1)
					{
						activeElem = &e;
						SDL_CaptureMouse(SDL_TRUE);
						
						e.liveState[0] = '1';
						
						if (e.doubleClickTimer > 0.f)
						{
							if (colorPicker.hasDefaultValue)
								e.value4 = colorPicker.defaultValue;
						}
						else
							e.doubleClickTimer = .2f;
					}
					else if (isInsideSlider2)
					{
						activeElem = &e;
						SDL_CaptureMouse(SDL_TRUE);
						
						e.liveState[0] = '2';
						
						if (e.doubleClickTimer > 0.f)
						{
							if (colorPicker.hasDefaultValue)
								e.value4 = colorPicker.defaultValue;
						}
						else
							e.doubleClickTimer = .2f;
					}
				}
			
				if (&e == activeElem && mouse.wentUp(BUTTON_LEFT))
				{
					e.liveState[0] = 0;
					
					activeElem = nullptr;
					SDL_CaptureMouse(SDL_FALSE);
				}
			
				if (&e == activeElem)
				{
					ControlSurfaceDefinition::Vector4 oldValue = e.value4;
					
					if (e.liveState[0] == 'p')
					{
						const float hue = saturate<float>((mouse.x - elem->x - picker_x) / float(picker_sx));
						const float lightness = saturate<float>(1.f - (mouse.y - elem->y) / float(elem->sy));
						
						e.value4.x = hue;
						e.value4.z = lightness;
					}
					else if (e.liveState[0] == '1')
					{
						const float saturation = saturate<float>(1.f - (mouse.y - elem->y) / float(elem->sy));
						
						e.value4.y = saturation;
					}
					else if (e.liveState[0] == '2')
					{
						const float white = saturate<float>(1.f - (mouse.y - elem->y) / float(elem->sy));
						
						e.value4.w = white;
					}
					
					if (e.value4 != oldValue)
					{
						e.valueHasChanged = true;
					}
				}
			}
		}
	}
	
	// send changed values over OSC
	
	char buffer[1200];
	osc::OutboundPacketStream s(buffer, 1200);
	int initialSize = 0;
	
	auto beginBundle = [&]()
	{
		s = osc::OutboundPacketStream(buffer, 1200);
		
		s << osc::BeginBundle();
		
		initialSize = s.Size();
	};
	
	auto sendBundle = [&]()
	{
		Assert(s.Size() != initialSize);
		
		try
		{
			s << osc::EndBundle;
			
			for (auto * oscSender : oscSenders)
				oscSender->send(s.Data(), s.Size());
		}
		catch (std::exception & e)
		{
			logError("%s", e.what());
		}
	};
	
	beginBundle();
	
	for (auto & e : elems)
	{
		if (e.valueHasChanged)
		{
			e.valueHasChanged = false;
			
			if (e.elem->type == ControlSurfaceDefinition::kElementType_Knob)
			{
				auto & knob = e.elem->knob;
				
				if (!knob.oscAddress.empty())
				{
					const float t = powf(activeElem->value, knob.exponential);
					const float value = knob.min * (1.f - t) + knob.max * t;
					
					if (s.Size() + knob.oscAddress.size() + 100 > 1200)
					{
						sendBundle();
						beginBundle();
					}
					
					s << osc::BeginMessage(knob.oscAddress.c_str());
					{
						s << value;
					}
					s << osc::EndMessage;
				}
			}
			else if (e.elem->type == ControlSurfaceDefinition::kElementType_Slider3)
			{
				auto & slider = e.elem->slider3;
				
				if (!slider.oscAddress.empty())
				{
					//const float t = powf(activeElem->value, knob.exponential);
					//const float value = knob.min * (1.f - t) + knob.max * t;
					const auto value = activeElem->value4;
					
					if (s.Size() + slider.oscAddress.size() + 100 > 1200)
					{
						sendBundle();
						beginBundle();
					}
					
					s << osc::BeginMessage(slider.oscAddress.c_str());
					{
						s << value.x << value.y << value.z;
					}
					s << osc::EndMessage;
				}
			}
			else if (e.elem->type == ControlSurfaceDefinition::kElementType_Listbox)
			{
				auto & listbox = e.elem->listbox;
				
				if (!listbox.oscAddress.empty())
				{
					const int index = clamp<int>((int)floorf(e.value), 0, listbox.items.size() -1);
					
					if (s.Size() + listbox.oscAddress.size() + listbox.items[index].size() + 100 > 1200)
					{
						sendBundle();
						beginBundle();
					}
					
					s << osc::BeginMessage(listbox.oscAddress.c_str());
					{
						s << listbox.items[index].c_str();
					}
					s << osc::EndMessage;
				}
			}
			else if (e.elem->type == ControlSurfaceDefinition::kElementType_ColorPicker)
			{
				auto & colorPicker = e.elem->colorPicker;
				
				if (!colorPicker.oscAddress.empty())
				{
					if (s.Size() + colorPicker.oscAddress.size() + sizeof(float) * 4 + 100 > 1200)
					{
						sendBundle();
						beginBundle();
					}
					
					s << osc::BeginMessage(colorPicker.oscAddress.c_str());
					{
						if (e.elem->colorPicker.colorSpace == ControlSurfaceDefinition::kColorSpace_Rgb)
						{
							s << e.value4.x;
							s << e.value4.y;
							s << e.value4.z;
						}
						else if (e.elem->colorPicker.colorSpace == ControlSurfaceDefinition::kColorSpace_Rgbw)
						{
							s << e.value4.x;
							s << e.value4.y;
							s << e.value4.z;
							s << e.value4.w;
						}
						else if (e.elem->colorPicker.colorSpace == ControlSurfaceDefinition::kColorSpace_Hsl)
						{
							s << e.value4.x;
							s << e.value4.y;
							s << e.value4.z;
						}
					}
					s << osc::EndMessage;
				}
			}
		}
	}
	
	if (s.Size() != initialSize)
		sendBundle();
}

void LiveUi::draw() const
{
	for (auto & e : elems)
	{
		auto * elem = e.elem;
		
		if (elem->type == ControlSurfaceDefinition::kElementType_Label)
		{
			auto & label = elem->label;
			
			setColor(40, 40, 40);
			drawText(elem->x, elem->y + elem->sy / 2.f, 12, +1, 0, "%s", label.text.c_str());
		}
		else if (elem->type == ControlSurfaceDefinition::kElementType_Knob)
		{
			hqBegin(HQ_FILLED_ROUNDED_RECTS);
			{
				if (&e == activeElem)
					setLumi(190);
				else if (&e == hoverElem)
					setLumi(210);
				else
					setLumi(200);
				hqFillRoundedRect(elem->x, elem->y, elem->x + elem->sx, elem->y + elem->sy, 4);
			}
			hqEnd();
			
			auto & knob = elem->knob;
			
			const float angle1 = (- 120 - 90) * float(M_PI) / 180.f;
			const float angle2 = (+ 120 - 90) * float(M_PI) / 180.f;
			
			const int numSteps = 100;
			
			const float radius = 14.f;
			
			const float midX = elem->x + elem->sx / 2.f;
			const float midY = elem->y + elem->sy / 2.f;
			
			hqBegin(HQ_LINES);
			{
				float x1;
				float y1;
				
				for (int i = 0; i < numSteps; ++i)
				{
					const float t = i / float(numSteps - 1);
					const float angle = angle1 * (1.f - t) + angle2 * t;
					
					float strokeSize;
					
					if (t < e.value)
					{
						setColor(200, 100, 100);
						strokeSize = 1.6f;
					}
					else
					{
						setColor(100, 100, 200);
						strokeSize = 1.f;
					}
					
					const float x2 = midX + cosf(angle) * radius;
					const float y2 = midY + sinf(angle) * radius;
					
					if (i != 0)
						hqLine(x1, y1, strokeSize, x2, y2, strokeSize);
					
					x1 = x2;
					y1 = y2;
				}
			}
			hqEnd();
			
			setColor(40, 40, 40);
			drawText(elem->x + elem->sx / 2.f, elem->y + elem->sy - 2, 10, 0, -1, "%s", knob.displayName.c_str());
		}
		else if (elem->type == ControlSurfaceDefinition::kElementType_Slider3)
		{
			hqBegin(HQ_FILLED_ROUNDED_RECTS);
			{
				if (&e == activeElem)
					setLumi(190);
				else if (&e == hoverElem)
					setLumi(210);
				else
					setLumi(200);
				hqFillRoundedRect(elem->x, elem->y, elem->x + elem->sx, elem->y + elem->sy, 4);
			}
			hqEnd();
			
			auto & slider = elem->slider3;
			
			setColor(40, 40, 40);
			drawText(elem->x + elem->sx / 2.f, elem->y + elem->sy/2 - 4, 10, 0, -1, "%s", slider.displayName.c_str());
			
			for (int i = 0; i < 3; ++i)
			{
				setColor(40, 40, 40);
				drawText(elem->x + elem->sx * (i + .5f) / 3, elem->y + elem->sy/2 + 4, 10, 0, +1, "%.2f", e.value4[i]);
			}
		}
		else if (elem->type == ControlSurfaceDefinition::kElementType_Listbox)
		{
			hqBegin(HQ_FILLED_ROUNDED_RECTS);
			{
				if (&e == activeElem)
					setLumi(190);
				else if (&e == hoverElem)
					setLumi(210);
				else
					setLumi(200);
				hqFillRoundedRect(elem->x, elem->y, elem->x + elem->sx, elem->y + elem->sy, 4);
			}
			hqEnd();
			
			auto & listbox = elem->listbox;
			
			setColor(40, 40, 40);
			setFont("calibri.ttf");
			
			if (listbox.items.empty() == false)
			{
				const int index = clamp<int>((int)floorf(e.value),  0, listbox.items.size() - 1);
				drawText(elem->x + elem->sx / 2.f, elem->y + elem->sy / 2.f, 10, 0, 0, "%s", listbox.items[index].c_str());
			}
			
			const float midY = elem->y + elem->sy / 2.f;
			
			hqBegin(HQ_FILLED_TRIANGLES);
			{
				setLumi(100);
				hqFillTriangle(elem->x + 8, midY - 4, elem->x + 4, midY, elem->x + 8, midY + 4);
				
				setLumi(100);
				hqFillTriangle(elem->x + elem->sx - 8, midY - 4, elem->x + elem->sx - 4, midY, elem->x + elem->sx - 8, midY + 4);
			}
			hqEnd();
		}
		else if (elem->type == ControlSurfaceDefinition::kElementType_ColorPicker)
		{
			const ControlSurfaceDefinition::ColorPicker & colorPicker = elem->colorPicker;
			
			const int picker_x = 0;
			const int picker_sx = elem->sx - 25;
			
			const int slider_x = elem->sx - 20;
			const int slider_sx = 20;
			
			const int slider1_x = elem->sx - 20;
			const int slider1_sx = 10;
			const int slider2_x = elem->sx - 10;
			const int slider2_sx = 10;
			
			// draw color picker
			
			const float saturation = e.value4.y;
			
			uint8_t colors[128][128][4];
			for (int y = 0; y < 128; ++y)
			{
				for (int x = 0; x < 128; ++x)
				{
					const float hue = (x + .5f) / 128.f;
					const float luminance = 1.f - (y + .5f) / 128.f;
					const Color color = Color::fromHSL(hue, saturation, luminance);
					colors[y][x][0] = color.r * 255.f;
					colors[y][x][1] = color.g * 255.f;
					colors[y][x][2] = color.b * 255.f;
					colors[y][x][3] = 255;
				}
			}
			
			GxTextureId texture = createTextureFromRGBA8(colors, 128, 128, true, true);
			
			hqSetTextureScreen(texture, elem->x + picker_x, elem->y, elem->x + picker_x + picker_sx, elem->y + elem->sy);
			{
				hqBegin(HQ_FILLED_ROUNDED_RECTS);
				{
					setLumi(255);
					hqFillRoundedRect(elem->x + picker_x, elem->y, elem->x + picker_x + picker_sx, elem->y + elem->sy, 4);
				}
				hqEnd();
			}
			hqClearTexture();
			
			freeTexture(texture);
			
			hqBegin(HQ_STROKED_ROUNDED_RECTS);
			{
				if (&e == activeElem)
					setLumi(190);
				else if (&e == hoverElem)
					setLumi(210);
				else
					setLumi(200);
				hqStrokeRoundedRect(elem->x + picker_x, elem->y, elem->x + picker_x + picker_sx, elem->y + elem->sy, 4, 2);
			}
			hqEnd();
			
			if (colorPicker.colorSpace == ControlSurfaceDefinition::kColorSpace_Hsl ||
				colorPicker.colorSpace == ControlSurfaceDefinition::kColorSpace_Rgb)
			{
				// draw saturation slider
				
				hqBegin(HQ_FILLED_ROUNDED_RECTS);
				{
					setLumi(100);
					hqFillRoundedRect(elem->x + slider_x, elem->y, elem->x + slider_x + slider_sx, elem->y + elem->sy, 4);
					
					setLumi(200);
					hqFillRoundedRect(elem->x + slider_x, elem->y + elem->sy * (1.f - e.value4.y), elem->x + slider_x + slider_sx, elem->y + elem->sy, 4);
				}
				hqEnd();
				
				setLumi(40);
				hqBegin(HQ_STROKED_ROUNDED_RECTS);
				{
					hqStrokeRoundedRect(elem->x + slider_x, elem->y, elem->x + slider_x + slider_sx, elem->y + elem->sy, 4, 2);
				}
				hqEnd();
			}
			else
			{
				// draw saturation slider
				
				hqBegin(HQ_FILLED_ROUNDED_RECTS);
				{
					setLumi(100);
					hqFillRoundedRect(elem->x + slider1_x, elem->y, elem->x + slider1_x + slider1_sx, elem->y + elem->sy, 4);
					
					setLumi(200);
					hqFillRoundedRect(elem->x + slider1_x, elem->y + elem->sy * (1.f - e.value4.y), elem->x + slider1_x + slider1_sx, elem->y + elem->sy, 4);
				}
				hqEnd();
				
				setLumi(40);
				hqBegin(HQ_STROKED_ROUNDED_RECTS);
				{
					hqStrokeRoundedRect(elem->x + slider1_x, elem->y, elem->x + slider1_x + slider1_sx, elem->y + elem->sy, 4, 2);
				}
				hqEnd();
				
				// draw white value slider
				
				hqBegin(HQ_FILLED_ROUNDED_RECTS);
				{
					setLumi(100);
					hqFillRoundedRect(elem->x + slider2_x, elem->y, elem->x + slider2_x + slider2_sx, elem->y + elem->sy, 4);
					
					setLumi(200);
					hqFillRoundedRect(elem->x + slider2_x, elem->y + elem->sy * (1.f - e.value4.w), elem->x + slider2_x + slider2_sx, elem->y + elem->sy, 4);
				}
				hqEnd();
				
				setLumi(40);
				hqBegin(HQ_STROKED_ROUNDED_RECTS);
				{
					hqStrokeRoundedRect(elem->x + slider2_x, elem->y, elem->x + slider2_x + slider2_sx, elem->y + elem->sy, 4, 2);
				}
				hqEnd();
			}
			
			// draw crosshair
			
			const float x = e.value4.x;
			const float y = 1.f - e.value4.z;
			setLumi(100);
			drawLine(
				elem->x + picker_x,
				elem->y + elem->sy * y,
				elem->x + picker_x + picker_sx,
				elem->y + elem->sy * y);
			drawLine(
				elem->x + picker_x + picker_sx * x,
				elem->y,
				elem->x + picker_x + picker_sx * x,
				elem->y + elem->sy);
			
			setColor(40, 40, 40);
			setFont("calibri.ttf");
			
			drawText(elem->x + elem->sx / 2.f, elem->y + elem->sy / 2.f, 10, 0, 0, "%s", colorPicker.displayName.c_str());
		}
		else if (elem->type == ControlSurfaceDefinition::kElementType_Separator)
		{
			hqBegin(HQ_FILLED_ROUNDED_RECTS);
			{
				setLumi(200);
				hqFillRoundedRect(elem->x, elem->y, elem->x + elem->sx, elem->y + elem->sy, 4);
			}
			hqEnd();
		}
	}
}

void LiveUi::drawTooltip() const
{
	gxPushMatrix();
	{
		gxTranslatef(mouse.x, mouse.y, 0);
		
		if (activeElem != nullptr)
		{
			auto * elem = activeElem->elem;
			
			if (elem->type == ControlSurfaceDefinition::kElementType_Knob)
			{
				auto & knob = elem->knob;
				
				hqBegin(HQ_FILLED_ROUNDED_RECTS);
				{
					setColor(200, 200, 100);
					hqFillRoundedRect(0, 0, 100, 30, 4);
				}
				hqEnd();
				
				const float t = powf(activeElem->value, knob.exponential);
				const float value = knob.min * (1.f - t) + knob.max * t;
				
				setLumi(20);
				drawText(10, 10, 20, +1, +1, "%.2f", value);
			}
		}
	}
	gxPopMatrix();
}

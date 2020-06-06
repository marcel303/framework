#include "colorwheel.h"
#include "particle_ui.h"
#include "particle.h"

#include "framework.h"
#include "Debugging.h"

#include <math.h>

// ui design
static const int kFontSize = 12;
static const int kCheckBoxHeight = 20;
static const int kCurveHeight = 80;
static const int kColorHeight = 20;
static const int kColorCurveHeight = 20;
#define kBackgroundFocusColor Color(0.f, 0.f, 1.f, g_uiState->opacity * .7f)
#define kBackgroundColor Color(0.f, 0.f, 0.f, g_uiState->opacity * .8f)
#define kBackgroundFocusColor2 Color(0.f, 0.f, 1.f, g_uiState->opacity * .9f)

// forward declarations
static float screenToCurve(const int x1, const int x2, const int x, const float offset);
static float curveToScreen(int x1, int x2, float t);

//

static ParticleCurve::Key * findNearestKey(ParticleCurve & curve, const float t, const float maxDeviation)
{
	ParticleCurve::Key * nearestKey = 0;
	float nearestDistance = 0.f;

	for (int i = 0; i < curve.numKeys; ++i)
	{
		const float dt = curve.keys[i].t - t;
		const float distance = sqrtf(dt * dt);

		if (distance < maxDeviation && (distance < nearestDistance || nearestKey == 0))
		{
			nearestKey = &curve.keys[i];
			nearestDistance = distance;
		}
	}

	return nearestKey;
}

void doParticleCurve(ParticleCurve & curve, const char * name)
{
	AssertMsg(g_menu != nullptr, "pushMenu must be called first!", 0);
	
	UiElem & elem = g_menu->getElem(name);
	
	enum Vars
	{
		kVar_SelectedKey,
		kVar_IsDragging,
		kVar_DragOffsetX,
		kVar_DragOffsetY
	};
	
	int & selectedKeyIndex = elem.getInt(kVar_SelectedKey, -1);
	bool & isDragging = elem.getBool(kVar_IsDragging, false);
	float & dragOffsetX = elem.getFloat(kVar_DragOffsetX, 0.f);
	float & dragOffsetY = elem.getFloat(kVar_DragOffsetY, 0.f);
	
	const float kMaxSelectionDeviation = 5 / float(g_menu->sx);
	
	const int kPadding = 5;

	const int x1 = g_drawX;
	const int x2 = g_drawX + g_menu->sx;
	const int y1 = g_drawY;
	const int y2 = g_drawY + kCurveHeight;

	g_drawY += kCurveHeight;
	
	if (g_doActions)
	{
		elem.tick(x1, y1, x2, y2);

		if (!elem.isActive)
			selectedKeyIndex = -1;
		else if (elem.hasFocus)
		{
			if (mouse.wentDown(BUTTON_LEFT))
			{
				// select or insert key

				const float t = screenToCurve(x1, x2, mouse.x, 0.f);
				const float value = clamp((mouse.y - y1) / float(y2 - y1), 0.f, 1.f);
				
				auto key = findNearestKey(curve, t, kMaxSelectionDeviation);
				
				if (key)
				{
					//
				}
				else
				{
					// insert a new key

					const float insertValue = value;

					key = curve.allocKey();
					key->t = t;
					key->value = insertValue;

					key = curve.sortKeys(key);
				}

				selectedKeyIndex = key - curve.keys;

				//
				
				isDragging = true;
				dragOffsetX = key->t - t;
				dragOffsetY = key->value - value;
			}
			
			if (!mouse.isDown(BUTTON_LEFT))
			{
				if (mouse.wentDown(BUTTON_RIGHT) || keyboard.wentDown(SDLK_DELETE) || keyboard.wentDown(SDLK_BACKSPACE))
				{
					selectedKeyIndex = -1;

					// erase key

					const float t = screenToCurve(x1, x2, mouse.x, 0.f);
					auto * key = findNearestKey(curve, t, kMaxSelectionDeviation);

					if (key)
					{
						curve.freeKey(key);
					}
				}
			}
		}

		if (isDragging)
		{
			if (elem.isActive && selectedKeyIndex != -1 && mouse.isDown(BUTTON_LEFT))
			{
				// move selected key around

				const float t = screenToCurve(x1, x2, mouse.x, dragOffsetX);
				const float value = screenToCurve(y1, y2, mouse.y, dragOffsetY);
				
				ParticleCurve::Key * selectedKey = curve.keys + selectedKeyIndex;

				selectedKey->t = t;
				selectedKey->value = value;

				selectedKey = curve.sortKeys(selectedKey);
				selectedKeyIndex = selectedKey - curve.keys;

				fassert(selectedKey->t == t);
			}
			else
			{
				isDragging = false;
			}
		}
	}
	
	if (g_doDraw)
	{
		if (elem.hasFocus)
			setColor(kBackgroundFocusColor);
		else
			setColor(kBackgroundColor);
		drawRect(x1, y1, x2, y2);
		
		hqBegin(HQ_LINES);
		{
			setColor(127, 127, 255);
			
			float value1 = curve.sample(0.f);
			float lx1 = x1;
			float ly1 = lerp(y1, y2, value1);
			
			for (int i = 1; i <= 100; ++i)
			{
				const float t = i / 100.f;
				
				const float value2 = curve.sample(t);
				const float lx2 = lerp(x1, x2, t);
				const float ly2 = lerp(y1, y2, value2);
				
				hqLine(lx1, ly1, 1.f, lx2, ly2, 1.f);
				
				value1 = value2;
				lx1 = lx2;
				ly1 = ly2;
			}
		}
		hqEnd();
		
		setDrawRect(x1, y1, x2-x1, y2-y1);
		{
			const float t = screenToCurve(x1, x2, mouse.x, 0.f);
			auto highlightedKey = isDragging ? &curve.keys[selectedKeyIndex] : findNearestKey(curve, t, kMaxSelectionDeviation);
			
			hqBegin(HQ_LINES);
			{
				for (int i = 0; i < curve.numKeys; ++i)
				{
					const float c =
						  !elem.hasFocus ? .5f
						: (highlightedKey == &curve.keys[i]) ? 1.f
						: (selectedKeyIndex == i) ? .8f
						: .5f;
					
					auto & key = curve.keys[i];
					
					const float x = lerp(x1, x2, key.t);
					
					setColorf(c/2, c/2, c);
					hqLine(x, y1, 2.f, x, y2, 2.f);
				}
			}
			hqEnd();
			
			for (int i = 0; i < curve.numKeys; ++i)
			{
				const float c =
					  !elem.hasFocus ? .5f
					: (highlightedKey == &curve.keys[i]) ? 1.f
					: (selectedKeyIndex == i) ? .8f
					: .5f;
				const float x = curveToScreen(x1, x2, curve.keys[i].t);
				const float y = curveToScreen(y1, y2, curve.keys[i].value);// (y1 + y2) / 2.f;
				drawUiCircle(x, y, 5.5f, c, c, c, 1.f);
			}
			
			if (highlightedKey != nullptr)
			{
				const float t = highlightedKey->t;
				const float value = highlightedKey->value;
				const float x = curveToScreen(x1, x2, t);
				const float y = curveToScreen(y1, y2, value);
				
				setColor(colorWhite);
				drawText(x, y + 10, 12, 0.f, +1.f, "(%.2f, %.2f)", t, value);
			}
		}
		clearDrawRect();
		
		setColor(colorWhite);
		drawText(x1 + kPadding, y1 + kPadding, kFontSize, +1.f, +1.f, "%s", name);
		
		setColor(colorBlue);
		drawRectLine(x1, y1, x2, y2);
	}
}

void doParticleColor(ParticleColor & color, const char * name)
{
	AssertMsg(g_menu != nullptr, "pushMenu must be called first!", 0);
	
	UiElem & elem = g_menu->getElem(name);
	
	const int kPadding = 5;
	const int kCheckButtonSize = kCheckBoxHeight - kPadding * 2;

	const int x1 = g_drawX;
	const int x2 = g_drawX + g_menu->sx;
	const int y1 = g_drawY;
	const int y2 = g_drawY + kColorHeight;

	g_drawY += kColorHeight;

	const int cx1 = x1;
	const int cy1 = y1;
	const int cx2 = x2;
	const int cy2 = y2;

	if (g_doActions)
	{
		const bool wasActive = elem.isActive;

		elem.tick(x1, y1, x2, y2);

		if (elem.isActive && elem.hasFocus && mouse.wentDown(BUTTON_LEFT))
		{
			g_uiState->activeColor = &color;

			if (!wasActive)
			{
				g_uiState->colorWheel->fromColor(
					g_uiState->activeColor->rgba[0],
					g_uiState->activeColor->rgba[1],
					g_uiState->activeColor->rgba[2],
					g_uiState->activeColor->rgba[3]);
			}
		}
	}

	if (g_doDraw)
	{
		if (elem.hasFocus)
			setColor(kBackgroundFocusColor);
		else
			setColor(kBackgroundColor);
		drawRect(x1, y1, x2, y2);
		
		setColor(colorWhite);
		drawUiRectCheckered(cx1, cy1, cx2, cy2, 4.f);
        setColorf(color.rgba[0], color.rgba[1], color.rgba[2], color.rgba[3]);
        drawRect(cx1, cy1, cx2, cy2);

		const float backgroundLumi = (color.rgba[0] + color.rgba[1] + color.rgba[2]) / 3.f;
		setColor(backgroundLumi < .5f ? colorWhite : colorBlack);
		drawText(x1 + kPadding + kCheckButtonSize + kPadding, (y1+y2)/2, kFontSize, +1.f, 0.f, "%s", name);
		
		setColor(colorBlue);
		drawRectLine(x1, y1, x2, y2);
	}
}

static float screenToCurve(const int x1, const int x2, const int x, const float offset)
{
	return saturate((x - x1) / float(x2 - x1) + offset);
}

static float curveToScreen(int x1, int x2, float t)
{
	return x1 + (x2 - x1) * t;
}

static ParticleColorCurve::Key * findNearestKey(ParticleColorCurve & curve, const float t, const float maxDeviation)
{
	ParticleColorCurve::Key * nearestKey = 0;
	float nearestDistance = 0.f;

	for (int i = 0; i < curve.numKeys; ++i)
	{
		const float dt = curve.keys[i].t - t;
		const float distance = sqrtf(dt * dt);

		if (distance < maxDeviation && (distance < nearestDistance || nearestKey == 0))
		{
			nearestKey = &curve.keys[i];
			nearestDistance = distance;
		}
	}

	return nearestKey;
}

void doParticleColorCurve(ParticleColorCurve & curve, const char * name)
{
	AssertMsg(g_menu != nullptr, "pushMenu must be called first!", 0);
	
	pushMenu(name);
	
	UiElem & elem = g_menu->getElem("curve");
	
	enum Vars
	{
		kVar_SelectedKey,
		kVar_IsDragging,
		kVar_DragOffset
	};
	
	int & selectedKeyIndex = elem.getInt(kVar_SelectedKey, -1);
	bool & isDragging = elem.getBool(kVar_IsDragging, false);
	float & dragOffset = elem.getFloat(kVar_DragOffset, 0.f);
	
	const float kMaxSelectionDeviation = 5 / float(g_menu->sx);

	const int x1 = g_drawX;
	const int x2 = g_drawX + g_menu->sx;
	const int y1 = g_drawY;
	const int y2 = g_drawY + kColorCurveHeight;

	g_drawY += kColorCurveHeight;

	const int cx1 = x1;
	const int cy1 = y1;
	const int cx2 = x2;
	const int cy2 = y2;

	if (g_doActions)
	{
		elem.tick(x1, y1, x2, y2);

		if (!elem.isActive)
			selectedKeyIndex = -1;
		else if (elem.hasFocus)
		{
			if (mouse.wentDown(BUTTON_LEFT))
			{
				// select or insert key

				const float t = screenToCurve(x1, x2, mouse.x, 0.f);
				auto key = findNearestKey(curve, t, kMaxSelectionDeviation);
				
				if (key)
				{
					g_uiState->colorWheel->fromColor(
						key->color.rgba[0],
						key->color.rgba[1],
						key->color.rgba[2],
						key->color.rgba[3]);
				}
				else
				{
					// insert a new key

					ParticleColor color;
					curve.sample(t, curve.useLinearColorSpace, color);

					key = curve.allocKey();
					key->color = color;
					key->t = t;

					g_uiState->colorWheel->fromColor(
						key->color.rgba[0],
						key->color.rgba[1],
						key->color.rgba[2],
						key->color.rgba[3]);

					key = curve.sortKeys(key);
				}

				selectedKeyIndex = key - curve.keys;

				isDragging = true;
				dragOffset = key->t - t;
			}
			
			if (!mouse.isDown(BUTTON_LEFT))
			{
				if (mouse.wentDown(BUTTON_RIGHT) || keyboard.wentDown(SDLK_DELETE) || keyboard.wentDown(SDLK_BACKSPACE))
				{
					selectedKeyIndex = -1;

					// erase key

					const float t = screenToCurve(x1, x2, mouse.x, 0.f);
					auto * key = findNearestKey(curve, t, kMaxSelectionDeviation);

					if (key)
					{
						curve.freeKey(key);
					}
				}
			}
		}

		if (isDragging)
		{
			if (elem.isActive && selectedKeyIndex != -1 && mouse.isDown(BUTTON_LEFT))
			{
				// move selected key around

				const float t = screenToCurve(x1, x2, mouse.x, dragOffset);

				ParticleColorCurve::Key * selectedKey = curve.keys + selectedKeyIndex;
				selectedKey->t = t;
				
				selectedKey = curve.sortKeys(selectedKey);
				selectedKeyIndex = selectedKey - curve.keys;

				fassert(selectedKey->t == t);
			}
			else
			{
				isDragging = false;
			}
		}

		if (elem.isActive)
		{
			g_uiState->activeColor = selectedKeyIndex != -1 ? &curve.keys[selectedKeyIndex].color : nullptr;
		}
	}

	if (g_doDraw)
	{
		if (elem.hasFocus)
			setColor(kBackgroundFocusColor);
		else
			setColor(kBackgroundColor);
		drawRect(x1, y1, x2, y2);
		setColor(colorBlue);
		drawRectLine(x1, y1, x2, y2);

		setColor(colorWhite);
		drawUiRectCheckered(cx1, cy1, cx2, cy2, 4.f);
		gxBegin(GX_QUADS);
		{
			for (int x = cx1; x < cx2; ++x)
			{
				const float t = (x - cx1 + .5f) / float(cx2 - cx1 - .5f);
				ParticleColor c;
				curve.sample(t, curve.useLinearColorSpace, c);
				gxColor4fv(c.rgba);
				gxVertex2f(x      , cy1);
				gxVertex2f(x + 1.f, cy1);
				gxVertex2f(x + 1.f, cy2);
				gxVertex2f(x      , cy2);
			}
		}
		gxEnd();
		
		{
			const float t = screenToCurve(x1, x2, mouse.x, 0.f);
			auto key = findNearestKey(curve, t, kMaxSelectionDeviation);
			for (int i = 0; i < curve.numKeys; ++i)
			{
				const float c =
					  !elem.hasFocus ? .5f
					: (key == &curve.keys[i]) ? 1.f
					: (selectedKeyIndex == i) ? .8f
					: .5f;
				const float x = curveToScreen(x1, x2, curve.keys[i].t);
				const float y = (y1 + y2) / 2.f;
				drawUiCircle(x, y, 5.5f, c, c, c, 1.f);
			}
		}

		//setColor(colorWhite);
		//drawText(x1 + kPadding + kCheckButtonSize + kPadding, y1, kFontSize, +1.f, +1.f, "%s", name);
	}
	
	doCheckBox(curve.useLinearColorSpace, "Linear Color Space", false);
	
	popMenu();
}

void doColorWheel(ParticleColor & color, const char * name, const float dt)
{
	doColorWheel(color.rgba[0], color.rgba[1], color.rgba[2], color.rgba[3], name, dt);
}

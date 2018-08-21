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

#include <algorithm>
#include "framework.h"

#define VIEW_SX (1920/2)
#define VIEW_SY (1080/2)

int main(int argc, char * argv[])
{
#if defined(CHIBI_RESOURCE_PATH)
	changeDirectory(CHIBI_RESOURCE_PATH);
#endif

	if (!framework.init(argc, (const char**)argv, VIEW_SX, VIEW_SY))
		return -1;
	
	Spriter spriter("character/Sprite.scml");
	SpriterState spriterState;
	
	spriterState.startAnim(spriter, "Idle");
	
	float direction = 0.f;
	
	spriterState.x = VIEW_SX/2;
	spriterState.y = VIEW_SY*3/4;
	spriterState.scale = .4f;
	
	float speedY = 0.f;
	
	bool isGrounded = true;
	
	while (!keyboard.isDown(SDLK_ESCAPE))
	{
		framework.process();
		
		if (isGrounded && ((rand() % 100) == 0 || keyboard.wentDown(SDLK_SPACE)))
		{
			isGrounded = false;
			speedY = -800.f;
			spriterState.startAnim(spriter, "Jump");
		}
		
		spriterState.x += direction * 400.f * framework.timeStep;
		
		speedY += 2000.f * framework.timeStep;
		
		spriterState.y += speedY * framework.timeStep;
		
		if (spriterState.y > VIEW_SY*3/4)
		{
			spriterState.y = VIEW_SY*3/4;
			speedY = 0.f;
			
			if (isGrounded == false)
			{
				isGrounded = true;
				spriterState.startAnim(spriter, "Walk");
			}
		}
		
		if (spriterState.x > VIEW_SX - 120 || keyboard.wentDown(SDLK_LEFT) || direction == 0.f)
		{
			spriterState.x = std::min(VIEW_SX - 120.f, spriterState.x);
			direction = -1.f;
			spriterState.flipX = false;
			if (isGrounded)
				spriterState.startAnim(spriter, "Walk");
		}
		
		if (spriterState.x < 120 || keyboard.wentDown(SDLK_RIGHT))
		{
			spriterState.x = std::max(120.f, spriterState.x);
			direction = +1.f;
			spriterState.flipX = true;
			if (isGrounded)
				spriterState.startAnim(spriter, "Walk");
		}
		
		spriterState.updateAnim(spriter, framework.timeStep);
		
		framework.beginDraw(200, 200, 200, 0);
		{
			hqBegin(HQ_LINES);
			setColor(100, 100, 100);
			hqLine(VIEW_SX/2-100, VIEW_SY*3/4 + 20, .3f, VIEW_SX/2+100, VIEW_SY*3/4 + 20, .5f);
			hqEnd();
			
			setColor(colorWhite);
			spriter.draw(spriterState);
			
			hqSetGradient(GRADIENT_LINEAR, Mat4x4(true).Scale(.1f, 1.f, 1.f).RotateZ(+M_PI_2).Translate(-VIEW_SX/2, -10, 0), Color(100, 100, 100), Color(200, 200, 200), COLOR_MUL);
			hqBegin(HQ_FILLED_ROUNDED_RECTS);
			hqFillRoundedRect(10, 10, VIEW_SX - 10, 100, 10.f);
			hqEnd();
			hqClearGradient();
			
			hqSetGradient(GRADIENT_LINEAR, Mat4x4(true).Scale(.1f, 1.f, 1.f).RotateZ(-M_PI_2).Translate(-VIEW_SX/2, -(VIEW_SY-10), 0), Color(100, 100, 100), Color(200, 200, 200), COLOR_MUL);
			hqBegin(HQ_FILLED_ROUNDED_RECTS);
			hqFillRoundedRect(10, VIEW_SY - 10, VIEW_SX - 10, VIEW_SY - 100, 10.f);
			hqEnd();
			hqClearGradient();
		}
		framework.endDraw();
	}
	
	framework.shutdown();
	
	return 0;
}

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
	setupPaths(CHIBI_RESOURCE_PATHS);

	if (!framework.init(VIEW_SX, VIEW_SY))
		return -1;
	
	// load the spriter file and create a new state for storing animation progress etc
	
	Spriter spriter("character/Sprite.scml");
	SpriterState spriterState;
	
	// begin the idle animation
	
	spriterState.startAnim(spriter, "Idle");
	
	// set the position and scale of the spriter
	
	spriterState.x = VIEW_SX/2;
	spriterState.y = VIEW_SY*3/4;
	spriterState.scale = .4f;
	
	// direction and (jumping) speed
	
	float direction = 0.f;
	float speedY = 0.f;
	
	// collision detection with the ground. if true, the character is standing on solid ground
	
	bool isGrounded = true;
	
	// keep running until the app is asked to quit
	
	while (!framework.quitRequested)
	{
		framework.process();
		
		// ask to quit the app when escape is pressed
		
		if (keyboard.wentDown(SDLK_ESCAPE))
			framework.quitRequested = true;
		
		// jump when grounded and SPACE is pressed. or randomly every so often
		
		if (isGrounded && ((rand() % 100) == 0 || keyboard.wentDown(SDLK_SPACE)))
		{
			isGrounded = false;
			speedY = -800.f;
			spriterState.startAnim(spriter, "Jump");
		}
		
		// movement
		
		spriterState.x += direction * 400.f * framework.timeStep;
		
		// apply gravity force to the speed and integrate position
		
		speedY += 2000.f * framework.timeStep;
		
		spriterState.y += speedY * framework.timeStep;
		
		// check for collision with the ground
		
		if (spriterState.y > VIEW_SY*3/4)
		{
			spriterState.y = VIEW_SY*3/4;
			speedY = 0.f;
			
			if (isGrounded == false)
			{
				// start the walking animation if we just transitioned to being grounded
				
				isGrounded = true;
				spriterState.startAnim(spriter, "Walk");
			}
		}
		
		// change direction when LEFT is pressed or when bumping into the right wall
		
		if (spriterState.x > VIEW_SX - 120 || keyboard.wentDown(SDLK_LEFT) || direction == 0.f)
		{
			spriterState.x = std::min(VIEW_SX - 120.f, spriterState.x);
			direction = -1.f;
			spriterState.flipX = false;
			if (isGrounded)
				spriterState.startAnim(spriter, "Walk");
		}
		
		// change direction when RIGHT is pressed or when bumping into the left wall
		
		if (spriterState.x < 120 || keyboard.wentDown(SDLK_RIGHT))
		{
			spriterState.x = std::max(120.f, spriterState.x);
			direction = +1.f;
			spriterState.flipX = true;
			if (isGrounded)
				spriterState.startAnim(spriter, "Walk");
		}
		
		// update spriter animation state
		
		spriterState.updateAnim(spriter, framework.timeStep);
		
		framework.beginDraw(200, 200, 200, 0);
		{
			// draw a dark-ish line to hint the ground
			
			hqBegin(HQ_LINES);
			setColor(100, 100, 100);
			hqLine(VIEW_SX/2-100, VIEW_SY*3/4 + 20, .3f, VIEW_SX/2+100, VIEW_SY*3/4 + 20, .5f);
			hqEnd();
			
			// draw the sprite
			
			setColor(colorWhite);
			spriter.draw(spriterState);
			
			// draw a nice looking gradient above and below the animation area
			
			hqSetGradient(GRADIENT_LINEAR, Mat4x4(true).Scale(.1f, 1.f, 1.f).RotateZ(+float(M_PI_2)).Translate(-VIEW_SX/2, -10, 0), Color(100, 100, 100), Color(200, 200, 200), COLOR_MUL);
			hqBegin(HQ_FILLED_ROUNDED_RECTS);
			hqFillRoundedRect(10, 10, VIEW_SX - 10, 100, 10.f);
			hqEnd();
			hqClearGradient();
			
			hqSetGradient(GRADIENT_LINEAR, Mat4x4(true).Scale(.1f, 1.f, 1.f).RotateZ(-float(M_PI_2)).Translate(-VIEW_SX/2, -(VIEW_SY-10), 0), Color(100, 100, 100), Color(200, 200, 200), COLOR_MUL);
			hqBegin(HQ_FILLED_ROUNDED_RECTS);
			hqFillRoundedRect(10, VIEW_SY - 10, VIEW_SX - 10, VIEW_SY - 100, 10.f);
			hqEnd();
			hqClearGradient();
			
			// show the help text
			
			setFont("calibri.ttf");
			setColor(80, 80, 80);
			drawText(VIEW_SX/2, 30, 16, 0, +1, "This example demonstrates loading, animating and drawing a character created using 2d skeletal animation software Spriter.");
		}
		framework.endDraw();
	}
	
	framework.shutdown();
	
	return 0;
}

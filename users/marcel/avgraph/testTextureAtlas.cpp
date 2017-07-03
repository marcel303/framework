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

#include "textureatlas.h"
#include "Timer.h"

extern const int GFX_SX;
extern const int GFX_SY;

void testTextureAtlas()
{
	{
		TextureAtlas ta;
		
		ta.init(128, 128, GL_R8, false, false, nullptr);
		
		bool success = true;
		
		uint8_t * values = (uint8_t*)malloc(1024 * 1024);
		memset(values, 0xff, 1024 * 1024);
		
		const uint64_t tr1 = g_TimerRT.TimeUS_get();
		
		success &= nullptr != ta.tryAlloc(values, 128, 128, GL_RED, GL_UNSIGNED_BYTE);
		
		success &= false != ta.makeBigger(256, 256);
		
		success &= nullptr != ta.tryAlloc(values, 128, 128, GL_RED, GL_UNSIGNED_BYTE);
		success &= nullptr != ta.tryAlloc(values, 128, 128, GL_RED, GL_UNSIGNED_BYTE);
		success &= nullptr != ta.tryAlloc(values, 128, 128, GL_RED, GL_UNSIGNED_BYTE);
		
		success &= false != ta.makeBigger(512, 256);
		
		for (int i = 0; i < 500; ++i)
		{
			success &= nullptr != ta.tryAlloc(values, random(4, 12), random(4, 12), GL_RED, GL_UNSIGNED_BYTE);
		}
		
		const uint64_t tr2 = g_TimerRT.TimeUS_get();
		
		printf("resize test: time=%.2fms, success=%d\n", (tr2 - tr1) / 1000.f, success ? 1 : 0);
		
		delete[] values;
		values = nullptr;
		
		do
		{
			framework.process();
			
			if (keyboard.wentDown(SDLK_o))
			{
				const uint64_t to1 = g_TimerRT.TimeUS_get();
				
				const bool success = ta.optimize();
				
				const uint64_t to2 = g_TimerRT.TimeUS_get();
				
				printf("optimize: time=%.2fms, success=%d\n", (to2 - to1) / 1000.f, success ? 1 : 0);
			}
			
			framework.beginDraw(0, 0, 0, 0);
			{
				pushBlend(BLEND_OPAQUE);
				gxSetTexture(ta.texture);
				{
					setColor(colorWhite);
					drawRect(0, 0, ta.a.sx, ta.a.sy);
				}
				gxSetTexture(0);
				popBlend();
				
			#if 1
				setColor(127, 0, 0);
				for (auto & e : ta.a.elems)
					if (e.isAllocated)
						drawRectLine(e.x, e.y, e.x + e.sx, e.y + e.sy);
			#endif
			}
			framework.endDraw();
		} while (!keyboard.wentDown(SDLK_SPACE));
	}
}

void testDynamicTextureAtlas()
{
	{
		BoxAtlas a;
		
		a.init(128, 128);
		
		bool success = true;
		
		const uint64_t tr1 = g_TimerRT.TimeUS_get();
		
		success &= nullptr != a.tryAlloc(128, 128);
		
		success &= false != a.makeBigger(256, 256);
		
		success &= nullptr != a.tryAlloc(128, 128);
		success &= nullptr != a.tryAlloc(128, 128);
		success &= nullptr != a.tryAlloc(128, 128);
		
		success &= false != a.makeBigger(512, 256);
		
		for (int i = 0; i < 500; ++i)
		{
			success &= nullptr != a.tryAlloc(random(4, 8), random(4, 8));
		}
		
		const uint64_t tr2 = g_TimerRT.TimeUS_get();
		
		printf("resize test: time=%.2fms, success=%d\n", (tr2 - tr1) / 1000.f, success ? 1 : 0);
	}
	
	BoxAtlas a;
	
	a.init(1024, 1024);
	
	const int kMaxElems = 1000;
	//const int kMaxElems = 100;
	
	BoxAtlasElem * elems[kMaxElems];
	int numElems = 0;
	
	const uint64_t t1 = g_TimerRT.TimeUS_get();
	
	for (int i = 0; i < kMaxElems; ++i)
	{
		const int sx = random(8, 32);
		const int sy = random(8, 32);
		
		BoxAtlasElem * e = a.tryAlloc(sx, sy);
		
		if (e != nullptr)
		{
			//logDebug("alloc %d, %d @ %d, %d", sx, sy, e->x, e->y);
			
			elems[numElems++] = e;
		}
		else
		{
			logDebug("failed to alloc %d, %d", sx, sy);
		}
	}
	
	const uint64_t t2 = g_TimerRT.TimeUS_get();
	
	printf("insert took %.2fms for %d elems (%d/%d)\n", (t2 - t1) / 1000.f, kMaxElems, numElems, kMaxElems);
	
	const uint64_t to1 = g_TimerRT.TimeUS_get();
	
	a.optimize();
	
	const uint64_t to2 = g_TimerRT.TimeUS_get();
	
	printf("optimize took %.2fms for %d elems (%d/%d)\n", (to2 - to1) / 1000.f, kMaxElems, numElems, kMaxElems);
	
	for (int i = 0; i < numElems; ++i)
	{
		a.free(elems[i]);
	}
	
	numElems = 0;
	
	//
	
	for (int i = 0; i < 6; ++i)
	{
		const int sx = random(64, 128);
		const int sy = random(64, 128);
		
		BoxAtlasElem * e = a.tryAlloc(sx, sy);
		
		if (e != nullptr)
		{
			logDebug("alloc %d, %d @ %d, %d", sx, sy, e->x, e->y);
			
			elems[numElems++] = e;
		}
		else
		{
			logDebug("failed to alloc %d, %d", sx, sy);
		}
	}
	
	for (int i = 0; i < numElems; ++i)
	{
		a.free(elems[i]);
	}
	
	numElems = 0;
	
	//
	
	TextureAtlas ta;
	
	ta.init(GFX_SX/2, GFX_SY/2, GL_R8, false, false, nullptr);
	
	const uint64_t tt1 = g_TimerRT.TimeUS_get();

	for (int i = 0; i < kMaxElems; ++i)
	{
		const int sx = random(8, 32);
		const int sy = random(8, 32);
		
		uint8_t * values = (uint8_t*)alloca(sx * sy * sizeof(uint8_t));
		
		for (int i = 0; i < sx * sy; ++i)
			values[i] = i;
		
		BoxAtlasElem * e = ta.tryAlloc(values, sx, sy, GL_RED, GL_UNSIGNED_BYTE);
		
		if (e != nullptr)
		{
			logDebug("alloc %d, %d @ %d, %d", sx, sy, e->x, e->y);
			
			elems[numElems++] = e;
		}
		else
		{
			logDebug("failed to alloc %d, %d", sx, sy);
		}
	}

	const uint64_t tt2 = g_TimerRT.TimeUS_get();
	
	printf("OpenGL insert took %.2fms for %d elems (%d/%d)\n", (tt2 - tt1) / 1000.f, kMaxElems, numElems, kMaxElems);
	
#if 1
	for (int i = 0; i < numElems; ++i)
	{
		ta.free(elems[i]);
	}
	
	numElems = 0;
#endif
	
	const int kAnimSize = 1000;
	int animItr = 0;
	
	do
	{
		framework.process();
		
		//
		
		if (mouse.isDown(BUTTON_LEFT))
			SDL_Delay(500);
		
		//
		
	#if 1
		if (animItr == kAnimSize)
		{
			for (int i = 0; i < numElems; ++i)
			{
				ta.free(elems[i]);
			}
			
			numElems = 0;
			animItr = 0;
		}
		
		const int numToAdd = mouse.x * 50 / GFX_SX;
		
	//for (int i = 0; i < kAnimSize; ++i)
	for (int i = 0; i < numToAdd; ++i)
		if (numElems < kMaxElems && animItr < kAnimSize)
		{
			const int sx = random(4, 100);
			const int sy = random(4, 100);
			
			uint8_t * values = (uint8_t*)alloca(sx * sy * sizeof(uint8_t));
			
			for (int i = 0; i < sx * sy; ++i)
				values[i] = i;
			
			BoxAtlasElem * e = ta.tryAlloc(values, sx, sy, GL_RED, GL_UNSIGNED_BYTE);
			
			if (e != nullptr)
			{
				logDebug("alloc %d, %d @ %d, %d", sx, sy, e->x, e->y);
				
				elems[numElems++] = e;
			}
			else
			{
				logDebug("failed to alloc %d, %d", sx, sy);
			}
			
			animItr++;
		}
	#endif
	
		if (keyboard.isDown(SDLK_o))
		{
			const uint64_t to1 = g_TimerRT.TimeUS_get();
			
			const bool success = ta.optimize();
			
			const uint64_t to2 = g_TimerRT.TimeUS_get();
			
			printf("optimize: time=%.2fms, success=%d\n", (to2 - to1) / 1000.f, success ? 1 : 0);
		}
		
		if (keyboard.isDown(SDLK_b))
		{
			const uint64_t tb1 = g_TimerRT.TimeUS_get();
			
			const bool success = ta.makeBigger(ta.a.sx + 4, ta.a.sy + 4);
			
			const uint64_t tb2 = g_TimerRT.TimeUS_get();
			
			printf("makeBigger: time=%.2fms, success=%d\n", (tb2 - tb1) / 1000.f, success ? 1 : 0);
		}
		
		//
		
		framework.beginDraw(0, 0, 0, 0);
		{
			pushBlend(BLEND_OPAQUE);
			gxSetTexture(ta.texture);
			{
				setColor(colorWhite);
				drawRect(0, 0, ta.a.sx, ta.a.sy);
			}
			gxSetTexture(0);
			popBlend();
			
		#if 1
			setColor(colorRed);
			for (auto & e : ta.a.elems)
				if (e.isAllocated)
					drawRectLine(e.x, e.y, e.x + e.sx, e.y + e.sy);
			
			BoxAtlas ao;
			ao.copyFrom(ta.a);
			
			ao.optimize();
			
			setColor(colorGreen);
			for (auto & e : ao.elems)
				if (e.isAllocated)
					drawRectLine(e.x, e.y, e.x + e.sx, e.y + e.sy);
		#endif
		}
		framework.endDraw();
	} while (!keyboard.wentDown(SDLK_SPACE));
	
	logDebug("done!");
}

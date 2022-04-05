#pragma once

#include "framework.h"

enum DrawBuffer
{
	DrawBuffer_Screen,
	DrawBuffer_Bitmap
};

static DrawBuffer g_DrawBuffer = DrawBuffer_Bitmap;

static uint32_t* g_Buffer = 0;
static uint32_t* g_Screen = 0;
static int g_BufferSx = 0;
static int g_BufferSy = 0;

static BOOL g_DrawStretch = FALSE;

static void Draw_Init(int sx, int sy)
{
	if (g_Buffer)
		delete[] g_Buffer;
	
	g_Buffer = new uint32_t[sx * sy];
	g_Screen = g_Buffer;
	
	g_BufferSx = sx;
	g_BufferSy = sy;
}

static void Draw_SetBuffer(DrawBuffer buffer)
{
	g_DrawBuffer = buffer;
}

static uint32_t* Draw_GetBuffer()
{
	switch (g_DrawBuffer)
	{
	case DrawBuffer_Screen:
		return g_Screen;
	case DrawBuffer_Bitmap:
		return g_Buffer;
	}

	return 0;
}

static void Draw_BeginScene()
{
	memset(Draw_GetBuffer(), 0, g_BufferSx * g_BufferSy * sizeof(uint32_t));
}

static void Draw_EndScene()
{
	GxTextureId texture = createTextureFromRGBA8(Draw_GetBuffer(), g_BufferSx, g_BufferSy, false, true);
	
	if (texture != 0)
	{
		pushBlend(BLEND_OPAQUE);
		gxSetTexture(texture, GX_SAMPLE_NEAREST, true);
		setColor(colorWhite);
		
		if (g_DrawStretch)
		{
			int viewSx;
			int viewSy;
			framework.getCurrentViewportSize(viewSx, viewSy);
			
			drawRect(0, 0, viewSx, viewSy);
		}
		else
		{
			drawRect(0, 0, g_BufferSx, g_BufferSy);
		}
		
		gxClearTexture();
		popBlend();
	}
	
	freeTexture(texture);
}

const float DRAW_SCALE = 10.0f;

const int g_DrawOpacity = 63;

static uint8_t g_DrawColor[3] = { 0xff, 0xff, 0xff };

static void Draw_SetColor(uint32_t color)
{
	g_DrawColor[0] = (color >> 16) & 0xff;
	g_DrawColor[1] = (color >>  8) & 0xff;
	g_DrawColor[2] = (color >>  0) & 0xff;
}

static void DrawPixel(int x, int y)
{
	if (x >= 0 && x < g_BufferSx &&
		y >= 0 && y < g_BufferSy)
	{
		uint8_t * __restrict rgba = (uint8_t*)(g_Buffer + x + g_BufferSx * y);
		
		int r = rgba[0] + g_DrawColor[0]; if (r > 255) r = 255;
		int g = rgba[1] + g_DrawColor[1]; if (g > 255) g = 255;
		int b = rgba[2] + g_DrawColor[2]; if (b > 255) b = 255;
		int a = rgba[3] + g_DrawOpacity; if (a > 255) a = 255;
		
		rgba[0] = r;
		rgba[1] = g;
		rgba[2] = b;
		rgba[3] = a;
	}
}

static void Draw_Line(float in_x1, float in_y1, float in_x2, float in_y2)
{
	int x1 = (int)roundf(in_x1);
	int y1 = (int)roundf(in_y1);
	int x2 = (int)roundf(in_x2);
	int y2 = (int)roundf(in_y2);
	
    int dx =  abs(x2 - x1);
    int dy = -abs(y2 - y1);
	
    int sx = x1 < x2 ? 1 : -1;
    int sy = y1 < y2 ? 1 : -1;
	
    int err = dx + dy;  /* error value e_xy */
	
    for (;;)
    {
        DrawPixel(x1, y1);
		
        if (x1 == x2 && y1 == y2)
        	break;
		
        int e2 = 2 * err;
		
        if (e2 >= dy) /* e_xy+e_x > 0 */
        {
            err += dy;
            x1 += sx;
        }
		
        if (e2 <= dx) /* e_xy+e_y < 0 */
        {
            err += dx;
            y1 += sy;
        }
    }
}

void Draw_Rect(float in_x1, float in_y1, float in_x2, float in_y2)
{
	const int x1 = (int)roundf(in_x1);
	const int y1 = (int)roundf(in_y1);
	const int x2 = (int)roundf(in_x2);
	const int y2 = (int)roundf(in_y2);
	
	for (int y = y1; y <= y2; ++y)
	{
		DrawPixel(x1, y);
		
		if (x2 != x1)
			DrawPixel(x2, y);
	}
	
	for (int x = x1 + 1; x < x2 - 1; ++x)
	{
		DrawPixel(x, y1);
		
		if (y2 != y1)
			DrawPixel(x, y2);
	}
}

static void Draw_Circle(float x, float y, float r)
{
#if 1
	Draw_Rect(x - r, y - r, x + r, y + r);
#else
	Draw_Begin();

	setColor(128, 128, 0);

	drawCircle(
		int(DRAW_SCALE * x),
		int(DRAW_SCALE * y),
		int(DRAW_SCALE * r),
		100);

	Draw_End();
#endif
}

static uint32_t makecol(int r, int g, int b)
{
	return (r << 16) | (g << 8) | b;
}

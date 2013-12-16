enum DrawBuffer
{
	DrawBuffer_Screen,
	DrawBuffer_Bitmap
};

static DrawBuffer g_DrawBuffer = DrawBuffer_Bitmap;

static BITMAP* g_Buffer = 0;
static BITMAP* g_Screen = 0;

static BOOL g_DrawStretch = FALSE;

static void Draw_Init(int sx, int sy)
{
	if (g_Buffer)
		destroy_bitmap(g_Buffer);
	if (g_Screen)
		destroy_bitmap(g_Screen);

	g_Buffer = create_bitmap(sx, sy);
	g_Screen = create_sub_bitmap(screen, 0, 0, sx, sy);
}

static void Draw_SetBuffer(DrawBuffer buffer)
{
	g_DrawBuffer = buffer;
}

static BITMAP* Draw_GetBuffer()
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
	clear_to_color(Draw_GetBuffer(), makecol(0, 0, 0));
}

static void Draw_EndScene()
{
	if (g_DrawBuffer != DrawBuffer_Screen)
	{
		scare_mouse();

		if (g_DrawStretch)
			stretch_blit(Draw_GetBuffer(), screen, 0, 0, g_Buffer->w, g_Buffer->h, 0, 0, SCREEN_W, SCREEN_H);
		else
			blit(Draw_GetBuffer(), screen, 0, 0, 0, 0, g_Buffer->w, g_Buffer->h);

		unscare_mouse();
	}
}

const float DRAW_SCALE = 10.0f;

const int g_DrawOpacity = 63;

static void Draw_Begin()
{
	drawing_mode(DRAW_MODE_TRANS, 0, 0, 0);
	set_add_blender(
		g_DrawOpacity,
		g_DrawOpacity, 
		g_DrawOpacity, 
		g_DrawOpacity);
}

static void Draw_End()
{
	solid_mode();
}

static void DrawLine(float x1, float y1, float x2, float y2)
{
	Draw_Begin();

	line(
		Draw_GetBuffer(),
		int(DRAW_SCALE * x1),
		int(DRAW_SCALE * y1),
		int(DRAW_SCALE * x2),
		int(DRAW_SCALE * y2),
		makecol(127, 127, 0));

	Draw_End();
}

void Draw_Rect(float x1, float y1, float x2, float y2, int c)
{
	Draw_Begin();

	rectfill(
		Draw_GetBuffer(),
		int(DRAW_SCALE * x1),
		int(DRAW_SCALE * y1),
		int(DRAW_SCALE * x2),
		int(DRAW_SCALE * y2),
		c);

	Draw_End();
}

static void DrawCircle(float x, float y, float r)
{
	Draw_Begin();

	circle(
		Draw_GetBuffer(),
		int(DRAW_SCALE * x),
		int(DRAW_SCALE * y),
		int(DRAW_SCALE * r),
		makecol(128, 128, 0));

	Draw_End();
}

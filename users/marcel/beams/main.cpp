#include <SDL/SDL.h>
#include "board.h"
#include "eventmgr.h"
#include "input.h"
#include "render.h"
#include "render_gl.h"

static TextureRGBA* CreateTexture(int size);
static TextureRGBA* LoadTexture_TGA(const char* fileName);

int main(int argc, char* argv[])
{
	gEventMgr = new EventMgr();

	gRender = new RenderGL();
	gRender->Init(320, 240);
	//gRender->Init(640, 640);

	//int size = 8;
	int size = 6;

	//TextureRGBA* texture = CreateTexture(512);
	//TextureRGBA* texture = CreateTexture(64);
	TextureRGBA* texture = LoadTexture_TGA("picture.tga");

	Board board;
	board.Setup(size, size, texture);

	delete texture;
	texture = 0;

	float dt = 1.0f / 60.0f;
	//float dt = 1.0f / 120.0f;

	bool stop = false;

	while (!gKeyboard.Get(KeyCode_Escape))
	{
#if 0
		if ((rand() % 20) == 0)
		{
			int x = rand() % size;
			int y = rand() % size;

			BoardTile* tile = board.Tile_get(x, y);
			tile->RotateX(1);
		}
		if ((rand() % 20) == 0)
		{
			int x = rand() % size;
			int y = rand() % size;

			BoardTile* tile = board.Tile_get(x, y);
			tile->RotateY(1);
		}
#endif
		if ((rand() % 30) == 0)
		{
			int coordIdx = rand() % 2;
			int coord[2];
			coord[coordIdx] = rand() % size;

			int direction = rand() % 2 ? -1 : +1;

			for (int i = 0; i < size; ++i)
			{
				int t = direction < 0 ? size - 1 - i : i;
				float delay = t / 10.0f;
				coord[1 - coordIdx] = i;
				BoardTile* tile = board.Tile_get(coord[0], coord[1]);
				if (coordIdx == 0)
					tile->RotateX(direction, delay);
				if (coordIdx == 1)
					tile->RotateY(direction, delay);
			}
		}

		board.Update(dt);

		//

		gRender->Clear();

		board.Render();

		gRender->Present();

		if (!board.IsAnimating_get())
		{
			LOG_INF("not animating - rest", 0);
			SDL_Delay(100);
		}
	}

	delete gRender;
	gRender = 0;

	delete gEventMgr;
	gEventMgr = 0;

	return 0;
}

static TextureRGBA* CreateTexture(int size)
{
	uint8_t* bytes = new uint8_t[size * size * 4];

	for (int y = 0; y < size; ++y)
	{
		for (int x = 0; x < size; ++x)
		{
			int index = (x + y * size) * 4;

			bytes[index + 0] = rand() % 256;
			bytes[index + 1] = x;
			bytes[index + 2] = y;
			bytes[index + 3] = 255;
		}
	}

	return new TextureRGBA(size, size, bytes, true);
}

#include "FileStream.h"
#include "ImageLoader_Tga.h"
#include "StreamReader.h"

TextureRGBA* LoadTexture_TGA(const char* fileName)
{
	FileStream stream;
	
	stream.Open(fileName, OpenMode_Read);
	
	StreamReader reader(&stream, false);
	
	TgaHeader header;
	
	header.Load(&stream);
	
	TgaLoader loader;
	
	uint8_t* bytes;
	
	loader.LoadData(&stream, header, &bytes);
	
	return new TextureRGBA(header.sx, header.sy, bytes, true);
}
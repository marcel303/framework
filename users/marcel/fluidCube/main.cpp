#include "fluidCube2d.h"
#include "fluidCube3d.h"
#include "fluidCube2d-gpu.h"
#include "framework.h"
#include "Timer.h"

static const int GFX_SX = 900;
static const int GFX_SY = 900;

//

struct FluidCube2dDemo
{
	FluidCube2d * cube = nullptr;

	GxTexture texture;
	
	void init()
	{
		cube = createFluidCube2d(300, 0.001f, 0.0001f, 1.f / 30.f);
		
		texture.allocate(cube->size, cube->size, GX_R32_FLOAT, true, true);
		texture.setSwizzle(0, 0, 0, GX_SWIZZLE_ONE);
	}
	
	void shut()
	{
		texture.free();
		
		delete cube;
		cube = nullptr;
	}
	
	void simulateAndDrawFrame()
	{
		const int SCALE = GFX_SX / cube->size;
		
		for (auto & d : cube->density)
			d *= .99f;
		
		for (int x = -4; x <= +4; ++x)
		{
			for (int y = -4; y <= +4; ++y)
			{
				cube->addDensity(mouse.x / SCALE + x, mouse.y / SCALE + y, .1f);
				cube->addVelocity(mouse.x / SCALE, mouse.y / SCALE, mouse.dx / 100.f, mouse.dy / 100.f);
			}
		}
		
		cube->step();
		
		// draw
		
		{
			gxPushMatrix();
			gxScalef(SCALE, SCALE, 1);
			
			texture.upload(cube->density.data(), 4, 0);
			gxSetTexture(texture.id);
			setColorClamp(false);
			setColor(2000, 2000, 2000);
			drawRect(0, 0, cube->size, cube->size);
			setColorClamp(true);
			gxSetTexture(0);
			
			pushBlend(BLEND_ADD);
			hqBegin(HQ_LINES);
			{
				setColor(30, 20, 10);
				
				for (int y = 0; y < cube->size; y += 4)
				{
					for (int x = 0; x < cube->size; x += 4)
					{
						const float vx = cube->Vx[cube->index(x, y)];
						const float vy = cube->Vy[cube->index(x, y)];
						
						hqLine(x, y, 1.f, x + vx * 300.f, y + vy * 300.f, 1.f);
					}
				}
			}
			hqEnd();
			popBlend();
			
			gxPopMatrix();
		}
	}
};

//

struct FluidCube3dDemo
{
	FluidCube3d * cube = nullptr;
	GxTexture texture;
	
	void init()
	{
		cube = createFluidCube3d(50, 0.0001f, 0.0001f, 1.f / 30.f);
		
		texture.allocate(cube->size, cube->size, GX_R32_FLOAT, true, true);
		texture.setSwizzle(0, 0, 0, GX_SWIZZLE_ONE);
	}

	void shut()
	{
		texture.free();
		
		delete cube;
		cube = nullptr;
	}

	void simulateAndDrawFrame()
	{
		const int SCALE = GFX_SX / cube->size;
		
		for (auto & d : cube->density)
			d *= .99f;
		
		const int z = cube->size/2 + cos(framework.time / 1.23f) * cube->size/3.f;
		cube->addDensity(mouse.x / SCALE, mouse.y / SCALE, z, 100.f);
		cube->addVelocity(mouse.x / SCALE, mouse.y / SCALE, z, mouse.dx, mouse.dy, cosf(framework.time) * 20.f);
		
		cube->step();
		
		// draw
		
		{
			projectPerspective3d(60.f, .01f, 100.f);
			gxTranslatef(0, 0, 2);
			gxRotatef(framework.time * 10.f, 0, 1, 0);
			
			setBlend(BLEND_ADD);
			setColor(colorWhite);
			setAlphaf(.4f);
			
			for (int z = 0; z < cube->size; ++z)
			{
				gxPushMatrix();
				gxTranslatef(0, 0, lerp<float>(-.5f, +.5f, z / float(cube->size - 1)));
				texture.upload(cube->density.data() + cube->index(0, 0, z), 4, 0);
				
				gxSetTexture(texture.id);
				drawRect(-.5f, -.5f, .5f, .5f);
				gxSetTexture(0);
				gxPopMatrix();
			}
			
			lineCube(Vec3(), Vec3(.5f, .5f, .5f));
			
			projectScreen2d();
		}
	}
};

//

struct FluidCube2dGpuDemo
{
	FluidCube2dGpu * cube = nullptr;
	
	void init()
	{
		cube = createFluidCube2dGpu(900, 0.0001f, 0.0001f, 1.f / 30.f);
	}
	
	void shut()
	{
		delete cube;
		cube = nullptr;
	}
	
	void simulateAndDrawFrame()
	{
		const int SCALE = GFX_SX / cube->size;
		
		cube->density.mulf(.99f, .99f, .99f);
		
		pushBlend(BLEND_ADD);
		pushSurface(&cube->density);
		{
			hqBegin(HQ_FILLED_CIRCLES);
			setColorf(.008f, 0.f, 0.f, 1.f / 100);
			for (int i = 0; i < 100; ++i)
				hqFillCircle(mouse.x / SCALE, mouse.y / SCALE, i * 60.f / 100);
			hqEnd();
		}
		popSurface();
		pushSurface(&cube->Vx);
		{
			hqBegin(HQ_FILLED_CIRCLES);
			setColorf(mouse.dx / 10.f, 0.f, 0.f, 1.f / 10);
			for (int i = 0; i < 10; ++i)
				hqFillCircle(mouse.x / SCALE, mouse.y / SCALE, i * 8.f / 10);
			hqEnd();
		}
		popSurface();
		pushSurface(&cube->Vy);
		{
			hqBegin(HQ_FILLED_CIRCLES);
			setColorf(mouse.dy / 10.f, 0.f, 0.f, 1.f / 10);
			for (int i = 0; i < 10; ++i)
				hqFillCircle(mouse.x / SCALE, mouse.y / SCALE, i * 8.f / 10);
			hqEnd();
		}
		popSurface();
		popBlend();
		
		for (int i = 0; i < 1; ++i)
		{
			cube->step();
		}
		
		// draw
		
		{
			gxPushMatrix();
			gxScalef(SCALE, SCALE, 1);
			
			pushBlend(BLEND_OPAQUE);
			{
				GxTextureId texture;
				
				if (keyboard.isDown(SDLK_s))
					texture = cube->s.getTexture();
				else if (keyboard.isDown(SDLK_v))
				{
					static int n = -1;
					if (keyboard.wentDown(SDLK_v))
						n = (n + 1) % 4;
					if (n == 0)
						texture = cube->Vx.getTexture();
					if (n == 1)
						texture = cube->Vy.getTexture();
					if (n == 2)
						texture = cube->Vx0.getTexture();
					if (n == 3)
						texture = cube->Vy0.getTexture();
				}
				else
					texture = cube->density.getTexture();
				
				setShader_TextureSwizzle(texture, 0, 0, 0, GX_SWIZZLE_ONE);
				setColor(4000, 3000, 2000);
				drawRect(0, 0, cube->size, cube->size);
				clearShader();
			}
			popBlend();
			
			gxPopMatrix();
		}
	}
};

//

int main(int argc, char * argv[])
{
	framework.allowHighDpi = false;
	
	if (!framework.init(GFX_SX, GFX_SY))
		return -1;
	
	FluidCube2dDemo cube2dDemo;
	FluidCube3dDemo cube3dDemo;
	FluidCube2dGpuDemo cube2dGpuDemo;
	
	cube2dDemo.init();
	cube3dDemo.init();
	cube2dGpuDemo.init();
	
	mouse.showCursor(false);
	
	int demo = 0;
	
	for (;;)
	{
		framework.process();

		if (framework.quitRequested)
			break;
		
		const int kButtonSy = 24;
		
		if (mouse.y < kButtonSy)
		{
			demo = mouse.x * 3 / GFX_SX;
		}
		
		framework.beginDraw(0, 0, 0, 0);
		{
			if (demo == 0)
				cube2dDemo.simulateAndDrawFrame();
			else if (demo == 1)
				cube2dGpuDemo.simulateAndDrawFrame();
			else
				cube3dDemo.simulateAndDrawFrame();
			
			const Color colorActive = Color::fromHSL(.3f, .5f, .8f);
			const Color colorInactive = Color::fromHSL(.3f, .5f, .5f);
			
			setColor(demo == 0 ? colorActive : colorInactive);
			drawRect(GFX_SX * 0/3, 0, GFX_SX * 1/3, kButtonSy);
			
			setColor(demo == 1 ? colorActive : colorInactive);
			drawRect(GFX_SX * 1/3, 0, GFX_SX * 2/3, kButtonSy);
			
			setColor(demo == 2 ? colorActive : colorInactive);
			drawRect(GFX_SX * 2/3, 0, GFX_SX * 3/3, kButtonSy);
		}
		framework.endDraw();
	}
	
	cube2dDemo.shut();
	cube3dDemo.shut();
	cube2dGpuDemo.shut();
	
	framework.shutdown();
	
	return 0;
}


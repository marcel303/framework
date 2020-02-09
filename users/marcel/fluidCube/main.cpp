#include "fluidCube2d.h"
#include "fluidCube3d.h"
#include "fluidCube2d-gpu.h"
#include "framework.h"
#include "Timer.h"

static const int GFX_SX = 800;
static const int GFX_SY = 400;

//

struct FluidCube2dDemo
{
	FluidCube2d * cube = nullptr;

	GxTexture texture;
	
	void init()
	{
		cube = createFluidCube2d(400, 200, .1f, .1f, 1.f / 30.f);
		
		texture.allocate(cube->sizeX, cube->sizeY, GX_R32_FLOAT, true, true);
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
		const int SCALE = GFX_SX / cube->sizeX;
		
		for (auto & d : cube->density)
			d *= .99f;
		
		for (int x = -4; x <= +4; ++x)
		{
			for (int y = -4; y <= +4; ++y)
			{
				cube->addDensity(mouse.x / SCALE + x, mouse.y / SCALE + y, .1f);
				cube->addVelocity(mouse.x / SCALE, mouse.y / SCALE, mouse.dx * 10.f, mouse.dy * 10.f);
			}
		}
		
		cube->step();
		
		// draw
		
		{
			gxPushMatrix();
			gxScalef(SCALE, SCALE, 1);
			
			const float * data = nullptr;
			
			if (keyboard.isDown(SDLK_s))
				data = cube->s.data();
			else if (keyboard.isDown(SDLK_v))
			{
				static int n = -1;
				if (keyboard.wentDown(SDLK_v))
					n = (n + 1) % 4;
				if (n == 0)
					data = cube->Vx.data();
				if (n == 1)
					data = cube->Vy.data();
				if (n == 2)
					data = cube->Vx0.data();
				if (n == 3)
					data = cube->Vy0.data();
			}
			else
				data = cube->density.data();
			
			texture.upload(data, 4, 0);
			gxSetTexture(texture.id);
			setColorClamp(false);
			if (keyboard.isDown(SDLK_v))
				setColor(10, 10, 10);
			else
				setColor(2000, 2000, 2000);
			drawRect(0, 0, cube->sizeX, cube->sizeY);
			setColorClamp(true);
			gxSetTexture(0);
			
			pushBlend(BLEND_ADD);
			hqBegin(HQ_LINES);
			{
				setColor(5, 10, 20);
				
				for (int y = 0; y < cube->sizeY; y += 4)
				{
					for (int x = 0; x < cube->sizeX; x += 4)
					{
						const int index = cube->index(x, y);
						
						const float vx = cube->Vx[index];
						const float vy = cube->Vy[index];
						const float d = cube->density[index];
						
					#if 0
						const float vLength = hypotf(vx, vy) + 1e-6f;
						
						//const float vScale = 1.f / fminf(vLength, 1.f) * 300.f;
						//const float vScale = 1.f / vLength * fminf(vLength, .04f) * 300.f;
						const float vScale = 1.f / vLength * fminf(vLength, .01f) * 3000.f;
						
						const float strokeSize = .4f + fminf(d * 20.f, 1.f) * 2.f;
						
						hqLine(x, y, strokeSize, x + vx * vScale, y + vy * vScale, 0.f);
					#elif 1
						const float vLength = sqrtf(vx * vx + vy * vy);
						
						//const float vScale = 1.f / fminf(vLength, 1.f) * 300.f;
						//const float vScale = 1.f / vLength * fminf(vLength, .04f) * 300.f;
						const float vScale = fminf(vLength, .01f) / vLength * 3000.f;
						
						hqLine(x, y, 2.f, x + vx * vScale, y + vy * vScale, 0.f);
					#elif 1
						hqLine(x, y, 2.f x + vx * 300.f, y + vy * 300.f, 0.f);
					#endif
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
		cube = createFluidCube3d(40, 40, 100, .01f, .01f, 1.f / 30.f);
		
		texture.allocate(cube->sizeX, cube->sizeY, GX_R32_FLOAT, true, true);
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
		const int SCALE_X = GFX_SX / cube->sizeX;
		const int SCALE_Y = GFX_SY / cube->sizeY;
		
		for (auto & d : cube->density)
			d *= .99f;
		
		const int z = cube->sizeZ/2 + cos(framework.time / 1.23f) * cube->sizeZ/3.f;
		cube->addDensity(mouse.x / SCALE_X, mouse.y / SCALE_Y, z, 100.f);
		cube->addVelocity(mouse.x / SCALE_X, mouse.y / SCALE_Y, z, mouse.dx * 100.f, mouse.dy * 100.f, cosf(framework.time) * 400.f);
		
		cube->step();
		
		// draw
		
		{
			projectPerspective3d(60.f, .01f, 100.f);
			gxTranslatef(0, 0, 1.7f);
			gxRotatef(framework.time * 10.f, 0, 1, 0);
			gxScalef(1, -1, 1);
			gxScalef(1.f /cube->sizeZ, 1.f / cube->sizeZ, 1.f /cube->sizeZ);
			
			setBlend(BLEND_ADD);
			setColor(250, 100, 30);
			setAlphaf(.4f);
			
			for (int z = 0; z < cube->sizeZ; ++z)
			{
				gxPushMatrix();
				gxTranslatef(0, 0, lerp<float>(-cube->sizeZ, +cube->sizeZ, z / float(cube->sizeZ - 1)));
				texture.upload(cube->density.data() + cube->index(0, 0, z), 4, 0);
				
				gxSetTexture(texture.id);
				drawRect(-cube->sizeX, -cube->sizeY, +cube->sizeX, +cube->sizeY);
				gxSetTexture(0);
				gxPopMatrix();
			}
			
			lineCube(Vec3(), Vec3(cube->sizeX, cube->sizeY, cube->sizeZ));
			
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
		cube = createFluidCube2dGpu(800, 400, 3, .01f, .001f, 1.f / 30.f);
		cube->iter = 10;
	}
	
	void shut()
	{
		delete cube;
		cube = nullptr;
	}
	
	void simulateAndDrawFrame()
	{
		const int SCALE = GFX_SX / cube->sizeX;
		
		for (int i = 0; i < cube->numPigments; ++i)
			cube->density[i].mulf(.997f, .997f, .997f);
		
		pushBlend(BLEND_ADD);
		for (int i = 0; i < cube->numPigments; ++i)
		{
			pushSurface(&cube->density[i]);
			{
				const int x = i/2 ? GFX_SX - mouse.x : mouse.x;
				const int y = i%2 ? GFX_SY - mouse.y : mouse.y;
				
				hqBegin(HQ_FILLED_CIRCLES);
				// note : we use cosine, and yes, this will add NAGATIVE density half of the time.. and it works, and it looks great!
				setColorf(cosf(framework.time * 4.f) * .02f, 0.f, 0.f, 1.f / 100);
				for (int i = 0; i < 100; ++i)
					hqFillCircle(x / SCALE, y / SCALE, i * 60.f / 100);
				hqEnd();
			}
			popSurface();
		}
		pushSurface(&cube->Vx);
		{
			hqBegin(HQ_FILLED_CIRCLES);
			setColorf(mouse.dx * 200.f, 0.f, 0.f, 1.f / 10);
			for (int i = 0; i < 10; ++i)
				hqFillCircle(mouse.x / SCALE, mouse.y / SCALE, i * 8.f / 10);
			hqEnd();
		}
		popSurface();
		pushSurface(&cube->Vy);
		{
			hqBegin(HQ_FILLED_CIRCLES);
			setColorf(mouse.dy * 200.f, 0.f, 0.f, 1.f / 10);
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
			
			if (!keyboard.isDown(SDLK_s) && !keyboard.isDown(SDLK_v))
			{
				pushBlend(BLEND_ADD);
				{
					for (int i = 0; i < cube->numPigments; ++i)
					{
						const GxTextureId texture = cube->density[i].getTexture();
						setShader_TextureSwizzle(texture, 0, 0, 0, GX_SWIZZLE_ONE);
						
						setColor(Color::fromHSL((i + 1.5f) / cube->numPigments, 1.f, .7f).mulRGB(10.f));
						drawRect(0, 0, cube->sizeX, cube->sizeY);
						clearShader();
					}
				}
				popBlend();
			}
			else
			{
				pushBlend(BLEND_OPAQUE);
				{
					GxTextureId texture;
					
					if (keyboard.isDown(SDLK_s))
						texture = cube->s[0].getTexture();
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
						texture = cube->density[0].getTexture();
					
					setShader_TextureSwizzle(texture, 0, 0, 0, GX_SWIZZLE_ONE);
					if (keyboard.isDown(SDLK_v))
						setColor(40, 30, 20);
					else
						setColor(4000, 3000, 2000);
					drawRect(0, 0, cube->sizeX, cube->sizeY);
					clearShader();
				}
				popBlend();
			}
			
			gxPopMatrix();
		}
	}
};

//

int main(int argc, char * argv[])
{
	setupPaths(CHIBI_RESOURCE_PATHS);
	
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
			
			Color colorActive = Color::fromHSL(.7f, .5f, .8f);
			Color colorInactive = Color::fromHSL(.7f, .5f, .5f);
			
			colorActive.a = .2f;
			colorInactive.a = .2f;
			
			setColor(demo == 0 ? colorActive : colorInactive);
			drawRect(GFX_SX * 0/3, 0, GFX_SX * 1/3, mouse.y < kButtonSy ? GFX_SY : kButtonSy);
			
			setColor(demo == 1 ? colorActive : colorInactive);
			drawRect(GFX_SX * 1/3, 0, GFX_SX * 2/3, mouse.y < kButtonSy ? GFX_SY : kButtonSy);
			
			setColor(demo == 2 ? colorActive : colorInactive);
			drawRect(GFX_SX * 2/3, 0, GFX_SX * 3/3, mouse.y < kButtonSy ? GFX_SY : kButtonSy);
		}
		framework.endDraw();
	}
	
	cube2dDemo.shut();
	cube3dDemo.shut();
	cube2dGpuDemo.shut();
	
	framework.shutdown();
	
	return 0;
}


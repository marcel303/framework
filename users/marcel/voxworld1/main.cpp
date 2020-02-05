#include "framework.h"
#include "Noise.h"

static const int kWorldSize = 64;

struct Voxel
{
	uint8_t type = 0;
};

struct World
{
	Voxel voxels[kWorldSize][kWorldSize][kWorldSize];
};

static const float kVoxelSize = .5f;

static bool is_voxel_hidden_from_view(const World & w, const int x, const int y, const int z)
{
	bool is_visible = false;
	
	is_visible |= w.voxels[x - 1][y][z].type == 0;
	is_visible |= w.voxels[x + 1][y][z].type == 0;
	is_visible |= w.voxels[x][y - 1][z].type == 0;
	is_visible |= w.voxels[x][y + 1][z].type == 0;
	is_visible |= w.voxels[x][y][z - 1].type == 0;
	is_visible |= w.voxels[x][y][z + 1].type == 0;
	
	return is_visible == false;
}

static void draw_world(const World & w)
{
	beginCubeBatch();
	{
		const Vec3 cube_size = Vec3(kVoxelSize, kVoxelSize, kVoxelSize) / 2.f;
		
		for (int x = 1; x < kWorldSize - 1; ++x)
		{
			for (int y = 1; y < kWorldSize - 1; ++y)
			{
				for (int z = 1; z < kWorldSize - 1; ++z)
				{
					if (w.voxels[x][y][z].type == 0)
						continue;
					
					if (is_voxel_hidden_from_view(w, x, y, z))
						continue;
					
					fillCube(
						Vec3(
							x * kVoxelSize,
							y * kVoxelSize,
							z * kVoxelSize),
						cube_size);
				}
			}
		}
	}
	endCubeBatch();
}

static bool is_inside_world(const int x, const int y, const int z)
{
	return
		x >= 0 && x < kWorldSize &&
		y >= 0 && y < kWorldSize &&
		z >= 0 && z < kWorldSize;
}

int main(int argc, char * argv[])
{
	setupPaths(CHIBI_RESOURCE_PATHS);
	
	framework.enableDepthBuffer = true;
	
    if (!framework.init(800, 600))
        return -1;

	World * w = new World();
	
	const Vec3 world_extents = Vec3(kWorldSize, kWorldSize, kWorldSize) * kVoxelSize;
	
	Camera3d camera;
	camera.position = world_extents / 2.f;
	
	bool physics_enabled = false;
	
	Vec3 speed;
	
	const float height = 1.f;
	
    for (;;)
    {
        framework.process();

        if (framework.quitRequested)
            break;
		
		camera.tick(framework.timeStep, true);
		
		const int x = (int)roundf(camera.position[0] / kVoxelSize);
		const int y = (int)roundf(camera.position[1] / kVoxelSize);
		const int z = (int)roundf(camera.position[2] / kVoxelSize);
		
		if (physics_enabled)
		{
			// add gravity
			
			speed[1] += -9.8f * framework.timeStep;
			
			camera.position += speed * framework.timeStep;
			
			// check for collision. warp ourselves up when we're grounded or would be grounded when we move up one voxel
			
			bool is_grounded = false;
			
			for (int y_offset = 0; y_offset < 2; ++y_offset)
			{
				if (is_inside_world(x, y + y_offset, z))
				{
					const bool is_solid = w->voxels[x][y + y_offset][z].type != 0;
					
					if (is_solid)
					{
						is_grounded = true;
						
						//camera.position[1] = (y + y_offset + .5f) * kVoxelSize;
						
						if (speed[1] < 0.f)
							speed[1] = -speed[1]*.2f;//0.f;
						else
							speed[1] += 12.f * framework.timeStep;
					}
				}
			}
			
			if (is_grounded && keyboard.wentDown(SDLK_SPACE))
			{
				speed[1] = -100.f;
			}
		}
		
		if (keyboard.isDown(SDLK_n))
		{
			for (int x = 0; x < kWorldSize; ++x)
			{
				for (int y = 0; y < kWorldSize; ++y)
				{
					for (int z = 0; z < kWorldSize; ++z)
					{
						const float value = scaled_octave_noise_4d(4, .5f, .03f, 0.f, 1.f, x, y, z, framework.time * 10.f);
						
						w->voxels[x][y][z].type = value < .7f ? 0x00 : 0xff;
					}
				}
			}
		}
		
		if (mouse.isDown(BUTTON_LEFT))
		{
			if (is_inside_world(x, y, z))
			{
				w->voxels[x][y][z].type = 0xff;
			}
		}
		
		if (keyboard.wentDown(SDLK_p))
		{
			physics_enabled = !physics_enabled;
		}
		
        framework.beginDraw(0, 0, 0, 0);
        {
        	projectPerspective3d(60.f, .01f, 100.f);
			
			camera.position[1] += 1.f;
			camera.pushViewMatrix();
			camera.position[1] -= 1.f;
			pushDepthTest(true, DEPTH_LESS);
			pushBlend(BLEND_OPAQUE);
			{
				pushShaderOutputs("n");
				draw_world(*w);
				popShaderOutputs();
				
				setColor(100, 100, 100);
				drawGrid3dLine(10, 10, 0, 2);
				
				{
					setColor(255, 0, 0);
					lineCube(world_extents/2.f, world_extents/2.f);
				}
			}
			popBlend();
			popDepthTest();
			camera.popViewMatrix();
        }
        framework.endDraw();
    }

	delete w;
	w = nullptr;
	
    framework.shutdown();

    return 0;
}

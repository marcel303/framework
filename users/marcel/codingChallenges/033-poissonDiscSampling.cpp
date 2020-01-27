#include "framework.h"
#include <vector>

/*
Coding Challenge #33: Poisson-disc Sampling
https://www.youtube.com/watch?v=flQgnCUxHlw
*/

const int GFX_SX = 800;
const int GFX_SY = 600;

const float r = 5.f;
const float w = r / sqrtf(2); // grid cell spacing. 2 = number of dimensions

const int k = 20;

int main(int argc, char * argv[])
{
	setupPaths(CHIBI_RESOURCE_PATHS);
	
	if (!framework.init(GFX_SX, GFX_SY))
		return -1;
	
	// create a grid large enough to store points for the entire screen
	
	const int grid_sx = floorf(GFX_SX / w);
	const int grid_sy = floorf(GFX_SY / w);
	const int grid_size = grid_sx * grid_sy;
	
	std::vector<Vec2> grid;
	grid.resize(grid_size);
	
	std::vector<float> grid_age; // keep track of age, to spice up the visuals
	grid_age.resize(grid_size);
	
	// create the active list
	
	std::vector<Vec2> active_list;
	
	auto restart = [&]()
	{
		// initialize the grid to -1
	
		for (int i = 0; i < grid_size; ++i)
			grid[i].Set(-1.f, -1.f);
		
		grid_age.clear();
		grid_age.resize(grid_size, 0.f);
		
		// clear the active list
		
		active_list.clear();
		
		// add a random starting point
		
		for (int i = 0; i < 2; ++i)
		{
			const float start_x = random<float>(0.f, GFX_SX);
			const float start_y = random<float>(0.f, GFX_SY);
			const Vec2 start_point = Vec2(start_x, start_y);
			const int grid_x = floorf(start_point[0] / w);
			const int grid_y = floorf(start_point[1] / w);
			const int grid_index = grid_x + grid_y * grid_sx;
			
			if (grid_index >= 0 && grid_index < grid_size)
			{
				grid[grid_index] = start_point;
				active_list.push_back(start_point);
			}
		}
	};
	
	restart();
	
	for (;;)
	{
		framework.process();
		
		if (framework.quitRequested)
			break;
		
		if (mouse.wentDown(BUTTON_LEFT))
			restart();
		
	for (int i = 0; i < 40; ++i)
		if (active_list.empty() == false)
		{
			// pick a random point from the active list
			
			const int index = rand() % active_list.size();
			const Vec2 origin = active_list[index];
			
			// remove the point from the active list
			
			std::swap(active_list[index], active_list.back());
			active_list.pop_back();
			
			// generate up to k new points, with a distance between r to 2r from the origin
			
			for (int i = 0; i < k; ++i)
			{
				const float angle = random<float>(0.f, float(M_PI) * 2.f);
				const float radius = random<float>(r, r * 2.f);
				const float x = cosf(angle) * radius;
				const float y = sinf(angle) * radius;
				const Vec2 point = origin + Vec2(x, y);
				
				const int grid_x = floorf(point[0] / w);
				const int grid_y = floorf(point[1] / w);
				
				if (grid_x < 0 || grid_x >= grid_sx ||
					grid_y < 0 || grid_y >= grid_sy)
				{
					continue;
				}
				
				bool is_free = true;
				
				for (int x_offset = -1; x_offset <= +1; ++x_offset)
				{
					for (int y_offset = -1; y_offset <= +1; ++y_offset)
					{
						const int neighbor_x = grid_x + x_offset;
						const int neighbor_y = grid_y + y_offset;
						
						if (neighbor_x < 0 || neighbor_x >= grid_sx ||
							neighbor_y < 0 || neighbor_y >= grid_sy)
						{
							continue;
						}
						
						const int neighbor_index = neighbor_x + neighbor_y * grid_sx;
						
						if (neighbor_index >= 0 && neighbor_index < grid_size)
						{
							const Vec2 & other_point = grid[neighbor_index];
							
							if (other_point[0] != -1.f)
							{
								const Vec2 delta = other_point - point;
								
								if (delta.CalcSize() < r)
									is_free = false;
							}
						}
					}
				}
				
				if (is_free)
				{
					const int grid_index = grid_x + grid_y * grid_sx;
					
					if (grid_index >= 0 && grid_index < grid_size)
					{
						Assert(grid[grid_index][0] == -1.f);
						
						grid[grid_index] = point;
						active_list.push_back(point);
					}
				}
			}
		}
		
		framework.beginDraw(0, 0, 0, 0);
		{
			setColor(100, 100, 100);
			hqBegin(HQ_FILLED_CIRCLES);
			{
				for (int i = 0; i < grid_size; ++i)
				{
					if (grid[i][0] == -1.f)
						continue;
					
					grid_age[i] += framework.timeStep;
					
					setAlphaf((6.f + sinf(grid_age[i] * 10.f)) / 7.f);
					hqFillCircle(grid[i][0], grid[i][1], 1.4f);
				}
			}
			hqEnd();
			
			setColor(255, 0, 255);
			hqBegin(HQ_FILLED_CIRCLES);
			{
				for (auto & pos : active_list)
				{
					hqFillCircle(pos[0], pos[1], 1.4f);
				}
			}
			hqEnd();
		}
		framework.endDraw();
	}
	
	grid.clear();
	
	framework.shutdown();
	
	return 0;
}

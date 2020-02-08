#include "framework.h"
#include <algorithm>
#include <map>
#include <queue>
#include <set>

/*
Coding Challenge #51: A* Path Finding
	Part 1: https://youtu.be/aKYlikFAV4k
	Part 2: https://youtu.be/EaZxUCWAjb0
	Part 3: https://youtu.be/jwRT4PCT6RU
*/

static const int kGridSx = 80;
static const int kGridSy = 60;

struct Cell
{
	bool blocked;
};

struct Grid
{
    Cell cells[kGridSx][kGridSy];
};

struct Coord
{
    int x;
    int y;
	
	// operator so we can use this object inside a set

    bool operator<(const Coord & other) const
    {
        if (x != other.x)
            return x < other.x;
        return y < other.y;
    }
};

struct AstarElem
{
    Coord coord;

	double cost;
    double cost_to_destination;

    // comparison operator so we can use this object inside a priority queue

    bool operator<(const AstarElem & other) const
    {
        return cost_to_destination > other.cost_to_destination;
    }
};

struct AstarVisitedNode
{
    Coord coord;
    Coord parent_coord;
};

static void randomizeGrid(Grid & grid)
{
    for (auto & column : grid.cells)
    {
        for (auto & cell : column)
            cell.blocked = (rand() % 3) == 0;
    }
}

int main(int argc, char * argv[])
{
	setupPaths(CHIBI_RESOURCE_PATHS);
	
    if (!framework.init(800, 600))
        return -1;

	// create a grid
	
    Grid grid;

    // create active priority queue and processed set

    std::priority_queue<AstarElem> active_queue;

    std::map<Coord, AstarVisitedNode> visited_set;
	
    std::set<Coord> processed_set;

    // create path finding state

    Coord destination_coord;

    bool reached = false;

    Coord current_coord;

	auto restart = [&]()
	{
		// reset current path finding state
		
		while (!active_queue.empty())
			active_queue.pop();
		
		visited_set.clear();
		
		processed_set.clear();
		
		// randomize the grid
		
		randomizeGrid(grid);
		
		// add the initial location for our path finding

		{
			AstarElem start_elem;
			start_elem.coord.x = 0;
			start_elem.coord.y = 0;
			start_elem.cost = 0.0;
			start_elem.cost_to_destination = 0.0;
			active_queue.push(start_elem);
			
			AstarVisitedNode visited_node;
			visited_node.coord = start_elem.coord;
			visited_node.parent_coord.x = -1;
			visited_node.parent_coord.y = -1;
			visited_set[visited_node.coord] = visited_node;
			
			//
			
			grid.cells[start_elem.coord.x][start_elem.coord.y].blocked = false;
		}
		
		destination_coord.x = kGridSx - 1;
    	destination_coord.y = kGridSy - 1;
		
    	grid.cells[destination_coord.x][destination_coord.y].blocked = false;
		
		reached = false;
		
		current_coord.x = -1;
    	current_coord.y = -1;
	};
	
	restart();
	
	// perform path finding, iteratively
	
	int speed = 8;
	
    for (;;)
    {
    	if (keyboard.wentDown(SDLK_a) && speed < 10)
    		speed++;
    	if (keyboard.wentDown(SDLK_z) && speed > 1)
    		speed--;
		
    	framework.process();
    	
        if (framework.quitRequested)
            break;

		if (mouse.wentDown(BUTTON_LEFT))
			restart();
		
		for (int i = 0; i < speed; ++i)
		{
			if (reached == false && active_queue.empty() == false)
			{
				auto elem = active_queue.top();
				active_queue.pop();

				current_coord = elem.coord;
				
				processed_set.insert(current_coord);
				
				if (current_coord.x == destination_coord.x &&
					current_coord.y == destination_coord.y)
				{
					reached = true;
				}
				else
				{
					for (int dx = -1; dx <= +1; ++dx)
					{
						for (int dy = -1; dy <= +1; ++dy)
						{
							if (dx == 0 && dy == 0)
								continue;

							const int new_x = elem.coord.x + dx;
							const int new_y = elem.coord.y + dy;
							
							if (new_x < 0 || new_x >= kGridSx || new_y < 0 || new_y >= kGridSy)
								continue;
							if (grid.cells[new_x][new_y].blocked)
								continue;
							
							AstarVisitedNode visited_node;
							visited_node.coord.x = new_x;
							visited_node.coord.y = new_y;
							visited_node.parent_coord = elem.coord;

							if (visited_set.count(visited_node.coord) == 0)
							{
								visited_set[visited_node.coord] = visited_node;

								const int left_x = destination_coord.x - new_x;
								const int left_y = destination_coord.y - new_y;
								
								AstarElem new_elem;
								new_elem.coord = visited_node.coord;
								new_elem.cost = elem.cost + hypot(dx, dy);
								new_elem.cost_to_destination = new_elem.cost + hypot(left_x, left_y);
								active_queue.push(new_elem);
							}
						}
					}
				}
			}
			else
			{
				restart();
			}
		}
		
        framework.beginDraw(0, 0, 0, 0);
        {
        	gxScalef(800 / float(kGridSx), 600 / float(kGridSy), 1);
			
			// draw grid
			
			hqBegin(HQ_FILLED_CIRCLES);
			{
				setColor(140, 140, 140);
				
				for (int x = 0; x < kGridSx; ++x)
					for (int y = 0; y < kGridSy; ++y)
						if (grid.cells[x][y].blocked)
							hqFillCircle(x + .5f, y + .5f, .3f);
			}
            hqEnd();
			
			// draw visited nodes. visibly, this will be only the active nodes, as the processed nodes, which is a subset of all visited nodes, are drawn on top of this using a different color
			
			hqBegin(HQ_FILLED_CIRCLES);
			{
				setColor(200, 255, 200);
				
				for (auto & visited_node_itr : visited_set)
				{
					auto & coord = visited_node_itr.first;

					hqFillCircle(coord.x + .5f, coord.y + .5f, .4f);
				}
			}
			hqEnd();
			
			// draw processed nodes
			
			hqBegin(HQ_FILLED_CIRCLES);
			{
				setColor(255/4, 255/5, 200/4);
				
				for (auto & coord : processed_set)
				{
					hqFillCircle(coord.x + .5f, coord.y + .5f, .4f);
				}
			}
			hqEnd();
			
            // draw nodes leading up to the current node

			hqBegin(HQ_LINES);
			{
				setColor(120, 40, 255);

				Coord coord = current_coord;

				for (;;)
				{
					auto i = visited_set.find(coord);

					if (i == visited_set.end())
						break;
					
					auto & node = i->second;
					
					if (node.parent_coord.x == -1)
						break;
					
					hqLine(node.coord.x + .5f, node.coord.y + .5f, .25f, node.parent_coord.x + .5f, node.parent_coord.y + .5f, .25f);
					
					coord = node.parent_coord;
				}
			}
			hqEnd();
			
			if (!active_queue.empty())
			{
				auto & top = active_queue.top();
				
				const int left_x = destination_coord.x - top.coord.x;
				const int left_y = destination_coord.y - top.coord.y;
				const float distance_to_destination = hypotf(left_x, left_y);
				
				setLumi(255);
				setAlphaf(powf(1.f - distance_to_destination / (hypotf(kGridSx, kGridSy) * 1.1f), 3.f));
				drawRect(0, 0, 800, 600);
			}
        }
        framework.endDraw();
    }

    framework.shutdown();

    return 0;
}

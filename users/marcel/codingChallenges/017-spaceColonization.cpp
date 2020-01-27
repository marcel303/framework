#include "framework.h"

/*
Coding Challenge #17: Fractal Trees - Space Colonization
https://www.youtube.com/watch?v=kKT0v3qhIQY
*/

static const float kMaxDist = 60.f;
static const float kMinDist = 4.f;

struct Leaf
{
	bool reached;
	
	float x;
	float y;
};

struct Branch
{
	int branch_count = 0;
	int parent_index;
	
	float x;
	float y;
	
	float direction_x;
	float direction_y;
	
	float force_x;
	float force_y;
	int force_count;
};

struct Tree
{
	std::vector<Leaf> leafs;
	
	std::vector<Branch> branches;
};

static void randomizeLeafs(Tree & tree)
{
	tree.leafs.resize(500);

	for (auto & leaf : tree.leafs)
	{
		leaf.reached = false;
		
		const float angle = random<float>(0.f, 2.f * float(M_PI));
		float distance = random<float>(0.f, 1.f);
		distance = powf(distance, .4f);
		distance *= 400.f;
		
		leaf.x = cosf(angle) * distance;
		leaf.y = sinf(angle) * distance;
	}
}

static void drawTree(const Tree & tree)
{
	hqBegin(HQ_FILLED_CIRCLES);
	{
		setLumi(200);
		
		for (auto & leaf : tree.leafs)
		{
			if (leaf.reached)
				continue;
			
			hqFillCircle(leaf.x, leaf.y, 2.f);
		}
	}
	hqEnd();
}

static void drawBranches(const Tree & tree)
{
	setColor(colorWhite);

	hqBegin(HQ_LINES);
	{
		for (auto & branch : tree.branches)
		{
			if (branch.parent_index == -1)
				continue;
			
			float sat = branch.parent_index / float(tree.branches.size());
			sat *= sat;
			
			setColor(Color::fromHSL(.5f, sat, .5f));
			
			auto & parent_branch = tree.branches[branch.parent_index];
			
			hqLine(branch.x, branch.y, 1.2f, parent_branch.x, parent_branch.y, 1.2f);
		}
	}
	hqEnd();
}

int main(int argc, char * argv[])
{
	setupPaths(CHIBI_RESOURCE_PATHS);
	
	if (!framework.init(800, 600))
		return -1;
	
	Tree tree;

	auto restart = [&]()
	{
		tree = Tree();
		
		randomizeLeafs(tree);

		Branch root;
		root.parent_index = -1;
		root.x = 0.f;
		root.y = 0.f;
		root.direction_x = 0.f;
		root.direction_y = -1.f;
		tree.branches.push_back(root);
	};
	
	restart();

	for (;;)
	{
		framework.process();
		
		if (framework.quitRequested)
			break;
		
		framework.timeStep = fminf(framework.timeStep, 1.f / 30.f);
		
		if (mouse.wentDown(BUTTON_LEFT))
		{
			restart();
		}
		
		auto grow = [](Tree & tree)
		{
			for (auto & branch : tree.branches)
			{
				branch.force_x = 0.f;
				branch.force_y = 0.f;
				branch.force_count = 0;
			}
			
			for (auto & leaf : tree.leafs)
			{
				if (leaf.reached)
					continue;
				
				Branch * closest_branch = nullptr;
				float closest_distance;
				
				for (auto & branch : tree.branches)
				{
					if (branch.branch_count >= 2)
						continue;
					
					const float dx = branch.x - leaf.x;
					const float dy = branch.y - leaf.y;
					const float d = hypotf(dx, dy);
					
					if (d < kMinDist)
					{
						leaf.reached = true;
						break;
					}
					else if (d > kMaxDist)
					{
						// ignore this branch
					}
					else if (closest_branch == nullptr || d < closest_distance)
					{
						closest_branch = &branch;
						closest_distance = d;
					}
				}
				
				if (leaf.reached)
					continue;
				
				if (closest_branch != nullptr)
				{
					const float dx = leaf.x - closest_branch->x;
					const float dy = leaf.y - closest_branch->y;
					const float d = hypotf(dx, dy);
					
					const float direction_x = dx / d / (d + .5f) * 1.1f;
					const float direction_y = dy / d / (d + .5f) * 1.1f;
					
					const float force_x = direction_x;
					const float force_y = direction_y;
					
					closest_branch->force_x += force_x;
					closest_branch->force_y += force_y;
					closest_branch->force_count++;
				}
			}
			
			const size_t num_branches = tree.branches.size();
			
			for (size_t i = 0; i < num_branches; ++i)
			{
				auto & branch = tree.branches[i];
				
				if (branch.force_count != 0)
				{
					branch.branch_count++;
					
					Branch next;
					next.parent_index = &branch - tree.branches.data();
					next.x = branch.x + branch.direction_x;
					next.y = branch.y + branch.direction_y;
					next.direction_x = branch.direction_x * .4f + branch.force_x / branch.force_count;
					next.direction_y = branch.direction_y * .4f + branch.force_y / branch.force_count;
					
					const float direction_mag = hypotf(next.direction_x, next.direction_y);
					next.direction_x /= direction_mag;
					next.direction_y /= direction_mag;
					
					next.x += next.direction_x;
					next.y += next.direction_y;
					
					tree.branches.push_back(next);
					
					//printf("adding branch.. %g, %g | %g, %g\n", next.x, next.y, next.direction_x, next.direction_y);
				}
			}
		};
		
		grow(tree);

		framework.beginDraw(0, 0, 0, 0);
		{
			gxTranslatef(400, 300, 0);
			
			//drawTree(tree);
			
			drawBranches(tree);
		}
		framework.endDraw();
	}
	
	framework.shutdown();
	
	return 0;
}


#include "framework.h"

/*
Coding Challenge #17: Fractal Trees - Space Colonization
https://www.youtube.com/watch?v=kKT0v3qhIQY
*/

static const float kSimScale = 10.f;

static const float kMaxDist = .6f * kSimScale;
static const float kMinDist = .04f * kSimScale;

struct Leaf
{
	bool reached;
	
	float x;
	float y;
	float z;
};

struct Branch
{
	int branch_energy = 0;
	int parent_index;
	
	float x;
	float y;
	float z;
	
	float direction_x;
	float direction_y;
	float direction_z;
	
	float force_x;
	float force_y;
	float force_z;
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
		
		for (;;)
		{
			const float x = random<float>(-1.f, +1.f);
			const float y = random<float>(-1.f, +1.f);
			const float z = random<float>(-1.f, +1.f);
			
			const float distance =
				sqrtf(
					x * x +
					y * y +
					z * z);
			
			if (distance > 1.f)
				continue;
			
			const float chance = powf(distance, 1.f / .4f);
			
			if (chance < random<float>(0.f, 1.f))
				continue;
			
			const float kDistanceMultiplier = kSimScale;
			
			leaf.x = x * kDistanceMultiplier;
			leaf.y = y * kDistanceMultiplier;
			leaf.z = z * kDistanceMultiplier;
			
			break;
		}
	}
}

static void drawTree(const Tree & tree)
{
	gxBegin(GX_POINTS);
	{
		setLumi(200);
		setAlpha(255);
		
		for (auto & leaf : tree.leafs)
		{
			if (leaf.reached)
				continue;
			
			gxVertex3f(leaf.x, leaf.y, leaf.z);
		}
	}
	gxEnd();
}

static void drawBranches(const Tree & tree)
{
	setColor(colorWhite);

	gxBegin(GX_LINES);
	{
		for (auto & branch : tree.branches)
		{
			if (branch.parent_index == -1)
				continue;
			
			float sat = branch.parent_index / float(tree.branches.size());
			sat *= sat;
			
			setColor(Color::fromHSL(.5f, sat, .5f));
			
			auto & parent_branch = tree.branches[branch.parent_index];
			
			gxVertex3f(
				branch.x,
				branch.y,
				branch.z);
			gxVertex3f(
				parent_branch.x,
				parent_branch.y,
				parent_branch.z);
		}
	}
	hqEnd();
}

int main(int argc, char * argv[])
{
	setupPaths(CHIBI_RESOURCE_PATHS);
	
	framework.vrMode = true;
	framework.enableVrMovement = true;
	framework.enableDepthBuffer = true;
	
	if (!framework.init(800, 600))
		return -1;
	
	Tree tree;

	auto restart = [&]()
	{
		tree = Tree();
		
		randomizeLeafs(tree);

		Branch root;
		root.branch_energy = 8;
		root.parent_index = -1;
		root.x = 0.f;
		root.y = 0.f;
		root.z = 0.f;
		root.direction_x = 0.f;
		root.direction_y = +1.f;
		root.direction_z = 0.f;
		tree.branches.push_back(root);
	};
	
	restart();

	for (;;)
	{
		framework.process();
		
		if (framework.quitRequested)
			break;
		
		framework.timeStep = fminf(framework.timeStep, 1.f / 30.f);
		
		if (mouse.wentDown(BUTTON_LEFT) || vrPointer[0].wentDown(VrButton_Trigger))
		{
			restart();
		}
		
		auto grow = [](Tree & tree)
		{
			for (auto & branch : tree.branches)
			{
				branch.force_x = 0.f;
				branch.force_y = 0.f;
				branch.force_z = 0.f;
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
					if (branch.branch_energy == 0)
						continue;
					
					const float dx = branch.x - leaf.x;
					const float dy = branch.y - leaf.y;
					const float dz = branch.z - leaf.z;
					const float d =
						sqrtf(
							dx * dx +
							dy * dy +
							dz * dz);
					
					if (d < kMinDist)
					{
						leaf.reached = true;
						branch.branch_energy += 1;
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
					const float dz = leaf.z - closest_branch->z;
					const float d =
						sqrtf(
							dx * dx +
							dy * dy +
							dz * dz);
					
					const float direction_x = dx / d / (d + .5f) * 1.1f;
					const float direction_y = dy / d / (d + .5f) * 1.1f;
					const float direction_z = dz / d / (d + .5f) * 1.1f;
					
					const float force_x = direction_x;
					const float force_y = direction_y;
					const float force_z = direction_z;
					
					closest_branch->force_x += force_x;
					closest_branch->force_y += force_y;
					closest_branch->force_z += force_z;
					closest_branch->force_count++;
				}
			}
			
			const size_t num_branches = tree.branches.size();
			
			const float direction_retain = .7f;
			
			for (size_t i = 0; i < num_branches; ++i)
			{
				auto & branch = tree.branches[i];
				
				if (branch.branch_energy == 0)
					continue;
					
				branch.branch_energy--;
				
				if (branch.force_count != 0)
				{
					Branch next;
					next.branch_energy = 1;
					next.parent_index = &branch - tree.branches.data();
					next.x = branch.x;
					next.y = branch.y;
					next.z = branch.z;
					
				#if 1
					next.direction_x = lerp<float>(branch.force_x / branch.force_count, branch.direction_x, direction_retain);
					next.direction_y = lerp<float>(branch.force_y / branch.force_count, branch.direction_y, direction_retain);
					next.direction_z = lerp<float>(branch.force_z / branch.force_count, branch.direction_z, direction_retain);
				#else
					next.direction_x = branch.direction_x * directionRetain + branch.force_x / branch.force_count;
					next.direction_y = branch.direction_y * directionRetain + branch.force_y / branch.force_count;
					next.direction_z = branch.direction_z * directionRetain + branch.force_z / branch.force_count;
				#endif
					
					const float direction_mag =
						sqrtf(
							next.direction_x * next.direction_x +
							next.direction_y * next.direction_y +
							next.direction_z * next.direction_z);
					next.direction_x /= direction_mag;
					next.direction_y /= direction_mag;
					next.direction_z /= direction_mag;
					 
					const float step_scale = kSimScale / 100.f; // todo : dt
					
					next.x += next.direction_x * step_scale;
					next.y += next.direction_y * step_scale;
					next.z += next.direction_z * step_scale;
					
					tree.branches.push_back(next);
					
					//printf("adding branch.. %g, %g, %g | %g, %g, %g\n", next.x, next.y, next.z, next.direction_x, next.direction_y, next.direction_z);
				}
			}
		};
		
		grow(tree);

		for (int i = 0; i < framework.getEyeCount(); ++i)
		{
			framework.beginEye(i, colorBlack);
			{
				pushDepthTest(false, DEPTH_LESS);
				
				pushBlend(BLEND_ALPHA);
				pushLineSmooth(true);
				{
					//drawTree(tree);
					
					drawBranches(tree);
				}
				popLineSmooth();
				popBlend();
				
				popDepthTest();
			}
			framework.endEye();
		}
		
		framework.present();
	}
	
	framework.shutdown();
	
	return 0;
}


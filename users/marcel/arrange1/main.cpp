#include "framework.h"
#include <list>

struct Arrangement
{
	struct Elem
	{
		int desired_x = 0;
		int desired_y = 0;
		
		int actual_x = 0;
		int actual_y = 0;
		
		int sx = 100;
		int sy = 100;

		float anim_x = 0.f;
		float anim_y = 0.f;
		
		float hue = 0.f;
	};

	std::list<Elem*> elems;

	void tick(const float dt)
	{
		const float retain = powf(.5f, dt * 60.f);

		for (auto * elem : elems)
		{
			elem->anim_x = lerp<float>(elem->actual_x, elem->anim_x, retain);
			elem->anim_y = lerp<float>(elem->actual_y, elem->anim_y, retain);
		}
	}

	void performLayout()
	{
		if (elems.empty() == false)
		{
			std::vector<Elem*> sortedElems;
			for (auto * elem : elems)
				sortedElems.push_back(elem);
			std::sort(sortedElems.begin(), sortedElems.end(),
				[](const Elem * e1, const Elem * e2)
				{
					return e1->desired_x < e2->desired_x;
				});
			
			auto * firstElem = sortedElems.front();
			
			int x = firstElem->desired_x;
			int y = firstElem->desired_y;
			
			for (auto * elem : sortedElems)
			{
				if (elem->desired_x > x)
					elem->actual_x = elem->desired_x;
				else
					elem->actual_x = x;
				
				elem->actual_y = elem->desired_y;
				
				x = elem->actual_x + elem->sx;
			}
		}
	}

	void draw() const
	{
		for (auto * elem : elems)
		{
			gxPushMatrix();
			{
				gxTranslatef(elem->anim_x, elem->anim_y, 0);

				setColor(Color::fromHSL(elem->hue, .5f, .8f));
				drawRect(0, 0, elem->sx, elem->sy);

				setColor(colorBlack);
				drawRectLine(0, 0, elem->sx, elem->sy);
			}
			gxPopMatrix();
		}
	}
};

enum State
{
	kState_Idle,
	kState_AddRect
};

int main(int argc, char * argv[])
{
	setupPaths(CHIBI_RESOURCE_PATHS);
	
	if (!framework.init(800, 600))
		return -1;

	State state = kState_Idle;

	Arrangement arr;

	Arrangement::Elem * elemToAdd = nullptr;

	for (;;)
	{
		framework.process();

		if (framework.quitRequested)
			break;

		if (state == kState_Idle)
		{
			if (mouse.wentDown(BUTTON_LEFT))
			{
				state = kState_AddRect;
				elemToAdd = new Arrangement::Elem();
				elemToAdd->sx = random<int>(40, 140);
				elemToAdd->sy = 70;
				elemToAdd->desired_x = mouse.x;
				elemToAdd->desired_y = 200;
				elemToAdd->hue = random<float>(0.f, 1.f);
				arr.elems.push_back(elemToAdd);
			}
		}
		else if (state == kState_AddRect)
		{
			elemToAdd->desired_x = mouse.x;

			if (mouse.wentUp(BUTTON_LEFT))
			{
				arr.performLayout();
				for (auto * elem : arr.elems)
				{
					elem->desired_x = elem->actual_x;
					elem->desired_y = elem->actual_y;
				}
				
				state = kState_Idle;
				elemToAdd = nullptr;
			}
		}

		arr.performLayout();

		arr.tick(framework.timeStep);

		framework.beginDraw(0, 0, 0, 0);
		{
			arr.draw();
		}
		framework.endDraw();
	}

	framework.shutdown();

	return 0;
}

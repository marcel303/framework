#include "framework.h"
#include <set>

void testRoutingEditor()
{
	std::vector<int> inputs;
	std::vector<bool> inputIsMulti;
	std::vector<bool> inputIsConnectedExternally;
	std::vector<int> outputs;

	for (int i = 0; i < 10; ++i)
	{
		inputs.push_back(rand() % 2);
		inputIsMulti.push_back((rand() % 2) == 0);
		inputIsConnectedExternally.push_back(true);
	}
	for (int i = 0; i < 6; ++i)
		outputs.push_back(rand() % 2);

	typedef std::tuple<int, int> Connection;

	std::set<Connection> connections;

	for (;;)
	{
		if (framework.quitRequested)
			exit(0);
		
		framework.process();
		
		framework.beginDraw(0, 0, 0, 0);
		{
			setFont("calibri.ttf");
			pushFontMode(FONT_SDF);
			
			const int gridX = 100;
			const int gridY = 100;
			
			gxTranslatef(gridX, gridY, 0);
			
			// draw the matrix
			
			setColor(colorWhite);
			
			const int cellSize = 24;
			
			const int mouseX = mouse.x - gridX;
			const int mouseY = mouse.y - gridY;
			
			const int cellX = mouseX / cellSize - (mouseX < 0 ? 1 : 0);
			const int cellY = mouseY / cellSize - (mouseY < 0 ? 1 : 0);
			
			const bool canConnect_atMouse =
				cellX >= 0 && cellX < inputs.size() &&
				cellY >= 0 && cellY < outputs.size() &&
				inputs[cellX] == outputs[cellY];
			
			auto c_atMouse = std::make_tuple(cellX, cellY);
			
			for (int x = 0; x < inputs.size(); ++x)
			{
				const int x1 = (x + 0) * cellSize;
				const int x2 = (x + 1) * cellSize;
				
				if (inputIsMulti[x] == false && inputIsConnectedExternally[x])
				{
					const int size = x == cellX ? 14 : 10;
					
					setLumi(100);
					hqBegin(HQ_FILLED_ROUNDED_RECTS);
					hqFillRoundedRect(x1 + 1, -size, x2 - 1, 10, 4.f);
					hqEnd();
				}
			}
			
			for (int y = 0; y < outputs.size(); ++y)
			{
				for (int x = 0; x < inputs.size(); ++x)
				{
					const int x1 = (x + 0) * cellSize;
					const int y1 = (y + 0) * cellSize;
					const int x2 = (x + 1) * cellSize;
					const int y2 = (y + 1) * cellSize;
					
					const bool isInside =
						mouseX >= x1 &&
						mouseY >= y1 &&
						mouseX < x2 &&
						mouseY < y2;
					
					bool highlight = false;
					highlight |= mouseX >= x1 && mouseX < x2;
					highlight |= mouseY >= y1 && mouseY < y2;
					highlight &= isInside == false;
					
					const bool canConnect = inputs[x] == outputs[y];
					
					auto c = std::make_tuple(x, y);
					
					const bool isConnected = connections.count(c) != 0;
					
					if (isInside && mouse.wentDown(BUTTON_LEFT))
					{
						if (isConnected)
							connections.erase(c);
						else if (canConnect)
						{
							if (inputIsMulti[x] == false)
							{
								inputIsConnectedExternally[x] = false;
								
								for (auto i = connections.begin(); i != connections.end(); )
								{
									auto & c = *i;
									
									if (std::get<0>(c) == x)
										i = connections.erase(i);
									else
										i++;
								}
							}
							
							connections.insert(c);
						}
					}
					
					int lumi = canConnect == false ? 150 : isInside ? 100 : 50;
					//if (highlight)
					//	lumi += 20;
					
					setLumi(lumi);
					drawRect(x1, y1, x2, y2);
				}
			}
			
			if (cellX >= 0 && cellX < inputs.size() && cellY >= 0 && cellY < outputs.size())
			{
				setColor(0, 127, 255, lerp<float>(.2f, .24f, (1.f + sinf(framework.time * 2.f)) / 2.f) * 255);
				
				{
					const int x1 = (cellX + 0) * cellSize;
					const int y1 = -1000;
					const int x2 = (cellX + 1) * cellSize;
					const int y2 = outputs.size() * cellSize;
					
					drawRect(x1, y1, x2, y2);
				}
				
				{
					const int x1 = -1000;
					const int y1 = (cellY + 0) * cellSize;
					const int x2 = inputs.size() * cellSize;
					const int y2 = (cellY + 1) * cellSize;
					
					drawRect(x1, y1, x2, y2);
				}
				
				setColor(colorWhite);
			}
			
			for (int y = 0; y < outputs.size(); ++y)
			{
				for (int x = 0; x < inputs.size(); ++x)
				{
					const int x1 = (x + 0) * cellSize;
					const int y1 = (y + 0) * cellSize;
					const int x2 = (x + 1) * cellSize;
					const int y2 = (y + 1) * cellSize;
					
					const bool isInside =
						mouseX >= x1 &&
						mouseY >= y1 &&
						mouseX < x2 &&
						mouseY < y2;
					
					auto c = std::make_tuple(x, y);
					
					const bool canConnect = inputs[x] == outputs[y];
					
					const bool isConnected = connections.count(c) != 0;
					
					if (isConnected)
					{
						if (x == cellX && y != cellY && inputIsMulti[x] == false && canConnect_atMouse && c_atMouse != c)
							setLumi(150);
						else
							setLumi(200);
						
						const float radius =
							c == c_atMouse
							? cellSize / 3.4f
							: cellSize / 3.f;
						
						hqBegin(HQ_FILLED_CIRCLES);
						hqFillCircle((x1 + x2)/2.f, (y1+y2)/2.f, radius);
						hqEnd();
					}
					else if (isInside && canConnect)
					{
						setLumi(150);
						
						const float radius = cellSize / 5.f;
						
						hqBegin(HQ_FILLED_CIRCLES);
						hqFillCircle((x1 + x2)/2.f, (y1+y2)/2.f, radius);
						hqEnd();
					}
					
					setLumi(100);
					drawRectLine(x1, y1, x2, y2);
				}
			}
			
			for (int x = 0; x < inputs.size(); ++x)
			{
				const int x1 = (x + 0) * cellSize;
				const int x2 = (x + 1) * cellSize;
				
				const int y = cellSize * outputs.size();
				
				setLumi(255);
				gxPushMatrix();
				gxTranslatef((x1 + x2) / 2.f, y, 0);
				gxRotatef(90, 0, 0, 1);
				drawText(+8, 0, x == cellX ? 16 : 14, +1, 0, "in %d", x);
				gxPopMatrix();
			}
			
			for (int y = 0; y < outputs.size(); ++y)
			{
				const int y1 = (y + 0) * cellSize;
				const int y2 = (y + 1) * cellSize;
				
				setLumi(255);
				drawText(-8, (y1 + y2) / 2.f, y == cellY ? 16 : 14, -1, 0, "out %d", y);
			}
			
			if (cellX >= 0 && cellX < inputs.size() && cellY >= 0 && cellY < outputs.size())
			{
				const bool canConnect = inputs[cellX] == outputs[cellY];
				
				if (canConnect)
				{
					setLumi(100);
					//drawText(cellSize * inputs.size() + 4, 0, 14, +1, +1, "out %d -> in %d", cellY, cellX);
				}
			}
			
			popFontMode();
		}
		framework.endDraw();
	}
}

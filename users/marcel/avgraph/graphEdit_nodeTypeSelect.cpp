/*
	Copyright (C) 2017 Marcel Smit
	marcel303@gmail.com
	https://www.facebook.com/marcel.smit981

	Permission is hereby granted, free of charge, to any person
	obtaining a copy of this software and associated documentation
	files (the "Software"), to deal in the Software without
	restriction, including without limitation the rights to use,
	copy, modify, merge, publish, distribute, sublicense, and/or
	sell copies of the Software, and to permit persons to whom the
	Software is furnished to do so, subject to the following
	conditions:

	The above copyright notice and this permission notice shall be
	included in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
	OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
	NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
	HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
	WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
	OTHER DEALINGS IN THE SOFTWARE.
*/

#include "framework.h"
#include "graph.h"
#include "graphEdit_nodeTypeSelect.h"
#include "../libparticle/ui.h"

typedef std::map<std::string, std::vector<const GraphEdit_TypeDefinition*>> TypeDefinitionsByCategory;

static void getTypeDefinitionsByCategory(const GraphEdit_TypeDefinitionLibrary & typeDefinitionLibrary, TypeDefinitionsByCategory & result)
{
	for (auto & typeDefinitonItr : typeDefinitionLibrary.typeDefinitions)
	{
		auto typeDefinition = &typeDefinitonItr.second;
		
		auto pos = typeDefinition->typeName.find('.');
		
		std::string category;
		
		if (pos == std::string::npos)
		{
			category = "other";
		}
		else
		{
			category = typeDefinition->typeName.substr(0, pos);
		}
		
		auto & list = result[category];
		
		list.push_back(typeDefinition);
	}
}

static void doBackground(const int x, const int y, const int sx, const int sy)
{
	UiElem & elem = g_menu->getElem("background");
	
	const int x1 = x;
	const int y1 = y;
	const int x2 = x1 + sx;
	const int y2 = y1 + sy;
	
	if (g_doActions)
	{
		elem.tick(x1, y1, x2, y2);
	}
	
	if (g_doDraw)
	{
		hqBegin(HQ_FILLED_ROUNDED_RECTS);
		{
			setColor(0, 0, 0, 200);
			hqFillRoundedRect(x1, y1, x2, y2, 12.f);
		}
		hqEnd();
	}
}

static const int kCategoryFontSize = 14;
static const int kCategoryPadding = 7;
static const int kCategoryHeight = 26;
static const int kCategorySpacing = 10;

static void measureCategoryButton(const char * name, int & sx, int & sy)
{
	float textSx;
	float textSy;
	measureText(kCategoryFontSize, textSx, textSy, name);
	
	sx = int(std::ceilf(textSx)) + kCategoryPadding * 2;
	sy = kCategoryHeight;
}

static bool doCategoryButton(const char * name, const bool isSelected)
{
	UiElem & elem = g_menu->getElem(name);
	
	int sx;
	int sy;
	measureCategoryButton(name, sx, sy);

	const int x1 = g_drawX;
	const int x2 = g_drawX + sx;
	const int y1 = g_drawY;
	const int y2 = g_drawY + sy;

	bool result = false;

	if (g_doActions)
	{
		elem.tick(x1, y1, x2, y2);

		if (elem.isActive && elem.hasFocus && mouse.wentUp(BUTTON_LEFT))
		{
			elem.deactivate();
			
			result = true;
		}
	}

	if (g_doDraw)
	{
		hqBegin(HQ_FILLED_ROUNDED_RECTS);
		{
			if (isSelected)
				setColor(255, 255, 255);
			else if (elem.hasFocus)
				setColor(255, 0, 127);
			else
				setColor(127, 0, 63);
			hqFillRoundedRect(x1, y1, x2, y2, kCategoryPadding);
		}
		hqEnd();
		
		if (isSelected)
			setColor(colorBlack);
		else
			setColor(colorWhite);
		drawText(x1 + kCategoryPadding, (y1+y2)/2, kCategoryFontSize, +1.f, 0.f, "%s", name);
	}
	
	g_drawX += sx;

	return result;
}

static bool doCategories(GraphEdit & graphEdit, TypeDefinitionsByCategory & categories, const int x, const int y, std::string & selectedCategoryName, std::string & selectedNodeType)
{
	bool result = false;
	
	pushMenu("categories", g_uiState->sx - 40);
	{
		g_drawX = x + 20;
		g_drawY = y + 20;
		
		auto begin = categories.begin();
		
		while (begin != categories.end())
		{
			auto end = begin;
			
			// see how many buttons we can fit on a line
			
			int totalSx = 0;
			
			// add the size for the first category button
			
			{
				auto & categoryName = begin->first;
				
				int sx;
				int sy;
				measureCategoryButton(categoryName.c_str(), sx, sy);
				
				totalSx += sx;
			}
			
			// keep checking if we can add the next category button without exceeding the width, until either it doesn't fit or we reached the end of the list
			
			for (;;)
			{
				end++;
				
				if (end == categories.end())
				{
					break;
				}
				else
				{
					auto & categoryName = end->first;
					
					int sx;
					int sy;
					measureCategoryButton(categoryName.c_str(), sx, sy);
					
					if (totalSx + kCategorySpacing + sx < g_menu->sx)
					{
						// the button fits. go check the next one
						
						totalSx += kCategorySpacing + sx;
					}
					else
					{
						// the button doesn't fit anymore. terminate the list
						
						break;
					}
				}
			}
			
			const int emptySpace = g_menu->sx - totalSx;
			const int padding = emptySpace / 2;
			
			int oldDrawX = g_drawX;
			
			g_drawX += padding;
			
			for (auto categoryItr = begin; categoryItr != end; ++categoryItr)
			{
				auto & categoryName = categoryItr->first;
				
				if (doCategoryButton(categoryName.c_str(), categoryName == selectedCategoryName))
				{
					selectedCategoryName = categoryName;
				}
				
				g_drawX += kCategorySpacing;
			}
			
			g_drawX = oldDrawX;
			g_drawY += kCategoryHeight + kCategorySpacing;
			
			begin = end;
		}
	}
	popMenu();
	
	pushMenu("nodeTypes", 200);
	{
		g_drawX += 20;
		g_drawY += 20;
		
		const int beginDrawY = g_drawY;
		
		int numRows = 0;
		
		if (selectedCategoryName.empty() == false)
		{
			auto & list = categories[selectedCategoryName];
			
			for (auto nodeType : list)
			{
				if (numRows == 10)
				{
					numRows = 0;
					
					g_drawX += g_menu->sx;
					g_drawX += 10;
					g_drawY = beginDrawY;
				}
				
				if (doButton(nodeType->typeName.c_str()))
				{
					selectedNodeType = nodeType->typeName;
					
					result = true;
				}
				
				numRows++;
			}
		}
	}
	popMenu();
	
	return result;
}

GraphEdit_NodeTypeSelect::GraphEdit_NodeTypeSelect()
	: x(0)
	, y(0)
	, sx(400)
	, sy(400)
	, uiState(nullptr)
	, selectedCategoryName()
{
	uiState = new UiState();
}

GraphEdit_NodeTypeSelect::~GraphEdit_NodeTypeSelect()
{
	delete uiState;
	uiState = nullptr;
}

bool GraphEdit_NodeTypeSelect::tick(GraphEdit & graphEdit, const GraphEdit_TypeDefinitionLibrary & typeDefinitionLibrary, const float dt, std::string & selectedNodeTypeName)
{
	bool result = false;
	
	makeActive(uiState, true, false);
	result = doMenus(graphEdit, typeDefinitionLibrary, selectedNodeTypeName);
	
	return result;
}

void GraphEdit_NodeTypeSelect::draw(const GraphEdit & graphEdit, const GraphEdit_TypeDefinitionLibrary & typeDefinitionLibrary)
{
	std::string selectedNodeTypeName;
	
	makeActive(uiState, false, true);
	doMenus(const_cast<GraphEdit&>(graphEdit), typeDefinitionLibrary, selectedNodeTypeName);
}

void GraphEdit_NodeTypeSelect::cancel()
{
	uiState->reset();
}

bool GraphEdit_NodeTypeSelect::doMenus(GraphEdit & graphEdit, const GraphEdit_TypeDefinitionLibrary & typeDefinitionLibrary, std::string & selectedNodeTypeName)
{
	bool result = false;
	
	pushMenu("nodeTypeSelect");
	pushFontMode(FONT_SDF);
	{
		uiState->sx = sx;
		
		TypeDefinitionsByCategory typeDefinitionsByCategory;
		getTypeDefinitionsByCategory(typeDefinitionLibrary, typeDefinitionsByCategory);
		
		if (g_doActions)
		{
			doBackground(x, y, sx, sy);
			
			result = doCategories(graphEdit, typeDefinitionsByCategory, x, y, selectedCategoryName, selectedNodeTypeName);
		}
		else
		{
			doBackground(x, y, sx, sy);
			
			doCategories(graphEdit, typeDefinitionsByCategory, x, y, selectedCategoryName, selectedNodeTypeName);
		}
	}
	popFontMode();
	popMenu();
	
	return result;
}

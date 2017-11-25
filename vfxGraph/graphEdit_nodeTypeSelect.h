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

#pragma once

#include <string>

struct GraphEdit;
struct GraphEdit_TypeDefinitionLibrary;
struct UiState;

struct GraphEdit_NodeTypeSelect
{
	int x;
	int y;
	int sx;
	int sy;
	
	UiState * uiState;

	std::string selectedCategoryName;

	GraphEdit_NodeTypeSelect();
	~GraphEdit_NodeTypeSelect();

	bool tick(
		GraphEdit & graphEdit,
		const GraphEdit_TypeDefinitionLibrary & typeDefinitionLibrary,
		const float dt,
		std::string & selectedNodeType);
	void draw(
		const GraphEdit & graphEdit,
		const GraphEdit_TypeDefinitionLibrary & typeDefinitionLibrary);
	
	void cancel();

	bool doMenus(
		GraphEdit & graphEdit,
		const GraphEdit_TypeDefinitionLibrary & typeDefinitionLibrary,
		std::string & selectedNodeTypeName);
};

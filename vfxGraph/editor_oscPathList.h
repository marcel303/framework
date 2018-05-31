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

#include "graph.h"

struct UiState;
struct VfxOscPathList;

struct ResourceEditor_OscPathList : GraphEdit_ResourceEditorBase
{
	UiState * uiState;
	VfxOscPathList * pathList;
	int learningIndex;
	
	ResourceEditor_OscPathList();
	virtual ~ResourceEditor_OscPathList() override;
	
	virtual void afterSizeChanged() override;
	virtual void afterPositionChanged() override;
	
	void doMenu(const float dt);
	
	bool tick(const float dt, const bool inputIsCaptured) override;
	void draw() const override;
	
	void setResource(const GraphNode & node, const char * type, const char * name) override;
	bool serializeResource(std::string & text) const override;
};

/*
	Copyright (C) 2020 Marcel Smit
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

#include <map>
#include <stdint.h>

struct Graph;
struct GraphEdit;
struct Graph_TypeDefinitionLibrary;
struct RealTimeConnection;
struct VfxGraph;

struct VfxGraphInstance
{
	VfxGraph * vfxGraph = nullptr;
	int sx = 0;
	int sy = 0;
	
	uint32_t texture = 0;
	
	RealTimeConnection * realTimeConnection = nullptr;
	
	~VfxGraphInstance();
};

struct VfxGraphManager
{
	virtual ~VfxGraphManager()
	{
	}
	
	virtual void selectFile(const char * filename) = 0;
	virtual void selectInstance(const VfxGraphInstance * instance) = 0;

	virtual VfxGraphInstance * createInstance(const char * filename, const int sx, const int sy) = 0;
	virtual void free(VfxGraphInstance *& instance) = 0;
	
	virtual void tick(const float dt) = 0;
	virtual void tickVisualizers(const float dt) = 0;
	
	virtual void traverseDraw() const = 0;
	
	virtual bool tickEditor(const int sx, const int sy, const float dt, const bool isInputCaptured) = 0;
	virtual void drawEditor(const int sx, const int sy) = 0;
};

struct VfxGraphManager_Basic : VfxGraphManager
{
	struct GraphCacheElem
	{
		bool isValid;
		Graph * graph;
		
		GraphCacheElem()
			: isValid(false)
			, graph(nullptr)
		{
		}
	};
	
	Graph_TypeDefinitionLibrary * typeDefinitionLibrary = nullptr;
	
	std::map<std::string, GraphCacheElem> graphCache;
	bool cacheOnCreate;
	
	std::vector<VfxGraphInstance*> instances;
	
	VfxGraphManager_Basic(const bool in_cacheOnCreate);
	virtual ~VfxGraphManager_Basic() override;
	
	void init();
	void shut();

	void addGraphToCache(const char * filename);
	
	virtual void selectFile(const char * filename) override;
	virtual void selectInstance(const VfxGraphInstance * instance) override;
	
	virtual VfxGraphInstance * createInstance(const char * filename, const int sx, const int sy) override;
	virtual void free(VfxGraphInstance *& instance) override;
	
	virtual void tick(const float dt) override;
	virtual void tickVisualizers(const float dt) override;

	virtual void traverseDraw() const override;
	
	virtual bool tickEditor(const int sx, const int sy, const float dt, const bool isInputCaptured) override;
	virtual void drawEditor(const int sx, const int sy) override;
};

struct VfxGraphFileRTC;

struct VfxGraphFile
{
	std::string filename;
	
	std::vector<VfxGraphInstance*> instances;
	
	const VfxGraphInstance * activeInstance = nullptr;
	
	VfxGraphFileRTC * realTimeConnection = nullptr;
	
	GraphEdit * graphEdit = nullptr;

	VfxGraphFile();
};

struct VfxGraphManager_RTE : VfxGraphManager
{
	int displaySx = 0;
	int displaySy = 0;
	
	Graph_TypeDefinitionLibrary * typeDefinitionLibrary = nullptr;
	
	std::map<std::string, VfxGraphFile*> files;
	
	VfxGraphFile * selectedFile = nullptr;
	
	VfxGraphManager_RTE(const int in_displaySx, const int in_displaySy);
	virtual ~VfxGraphManager_RTE() override;
	
	void init();
	void shut();
	
	virtual void selectFile(const char * filename) override;
	virtual void selectInstance(const VfxGraphInstance * instance) override;
	
	virtual VfxGraphInstance * createInstance(const char * filename, const int sx, const int sy) override;
	virtual void free(VfxGraphInstance *& instance) override;
	
	virtual void tick(const float dt) override;
	virtual void tickVisualizers(const float dt) override;
	
	virtual void traverseDraw() const override;
	
	virtual bool tickEditor(const int sx, const int sy, const float dt, const bool isInputCaptured) override;
	virtual void drawEditor(const int sx, const int sy) override;
};

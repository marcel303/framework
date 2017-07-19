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
#include <map>
#include <string>
#include <vector>

#if defined(DEBUG)
	#define AUDIO_GRAPH_ENABLE_TIMING 0
#else
	#define AUDIO_GRAPH_ENABLE_TIMING 0
#endif

struct AudioBuffer;
struct AudioGraph;
struct AudioNodeBase;
struct AudioOutputChannel;
struct AudioPlug;

struct AudioGraph
{
	struct ValueToFree
	{
		enum Type
		{
			kType_Unknown,
			kType_Bool,
			kType_Int,
			kType_Float,
			kType_String,
			kType_AudioValue
		};
		
		Type type;
		void * mem;
		
		ValueToFree()
			: type(kType_Unknown)
			, mem(nullptr)
		{
		}
		
		ValueToFree(const Type _type, void * _mem)
			: type(_type)
			, mem(_mem)
		{
		}
	};
	
	std::map<GraphNodeId, AudioNodeBase*> nodes;
	
	GraphNodeId displayNodeId;
	
	mutable int nextTickTraversalId;
	mutable int nextDrawTraversalId;
	
	Graph * graph; // todo : remove ?
	
	std::vector<ValueToFree> valuesToFree;
	
	double time;
	
	AudioGraph();
	~AudioGraph();
	
	void destroy();
	void connectToInputLiteral(AudioPlug & input, const std::string & inputValue);
	
	void tick(const float dt, const bool traverseUnreferenced);
	void draw(AudioOutputChannel * outputChannels, const int numOutputChannels, const bool traverseUnreferenced) const;
};

//

AudioNodeBase * createAudioNode(const GraphNodeId nodeId, const std::string & typeName, AudioGraph * audioGraph);

AudioGraph * constructAudioGraph(const Graph & graph, const GraphEdit_TypeDefinitionLibrary * typeDefinitionLibrary);

#pragma once

#include <string>
#include <vector>

struct StructuredType;
struct TypeDB;

namespace ControlSurfaceDefinition
{
	enum ElementType
	{
		kElementType_None,
		kElementType_Knob
	};

	struct Knob
	{
		std::string name;
		float defaultValue = 0.f;
		bool hasDefaultValue = false;
		float min = 0.f;
		float max = 1.f;
		float exponential = 1.f;
		std::string oscAddress;
	};
	
	struct Element
	{
		ElementType type = kElementType_None;
		
		int x = 0;
		int y = 0;
		int sx = 0;
		int sy = 0;
		
		Knob knob;
		
		void makeKnob()
		{
			type = kElementType_Knob;
			
			sx = 40;
			sy = 40;
		}
	};

	struct Group
	{
		std::string name;
		std::vector<Element> elems;
	};
	
	struct SurfaceLayout
	{
		int sx = 0;
		int sy = 0;
		
		int marginX = 0;
		int marginY = 0;
		
		int paddingX = 0;
		int paddingY = 0;
	};
	
	struct Surface
	{
		std::vector<Group> groups;
		
		SurfaceLayout layout;
		
		void initializeDefaultValues();
		void performLayout();
	};
	
	void reflect(TypeDB & typeDB);
}

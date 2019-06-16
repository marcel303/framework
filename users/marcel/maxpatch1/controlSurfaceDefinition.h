#pragma once

#include <string>
#include <vector>

struct StructuredType;
struct TypeDB;

namespace ControlSurfaceDefinition
{
	struct Element;
	
	enum ElementType
	{
		kElementType_None,
		kElementType_Label,
		kElementType_Knob,
		kElementType_Listbox,
		kElementType_Separator
	};
	
	struct Label
	{
		std::string text;
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
	
	struct Listbox
	{
		std::string name;
		std::vector<std::string> items;
		std::string defaultValue;
		bool hasDefaultValue = false;
		std::string oscAddress;
	};
	
	struct Element
	{
		ElementType type = kElementType_None;
		
		int x = 0;
		int y = 0;
		int sx = 0;
		int sy = 0;
		
		bool divideLeft = false;
		bool divideRight = false;
		bool divideBottom = false;
		
		Label label;
		
		Knob knob;
		
		Listbox listbox;
		
		void makeLabel()
		{
			type = kElementType_Label;
			
			sx = 100;
			sy = 16;
		}
		
		void makeKnob()
		{
			type = kElementType_Knob;
			
			sx = 40;
			sy = 40;
		}
		
		void makeListbox()
		{
			type = kElementType_Listbox;
			
			sx = 100;
			sy = 20;
		}
		
		void makeSeparator()
		{
			type = kElementType_Separator;
			
			sx = 6;
			sy = 6;
			
			divideLeft = true;
			divideRight = true;
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

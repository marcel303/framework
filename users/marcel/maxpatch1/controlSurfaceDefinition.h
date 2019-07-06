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
		kElementType_Slider2,
		kElementType_Slider3,
		kElementType_Listbox,
		kElementType_ColorPicker,
		kElementType_Separator
	};
	
	enum Unit
	{
		kUnit_Int,
		kUnit_Float,
		kUnit_Time,
		kUnit_Hertz,
		kUnit_Decibel,
		kUnit_Percentage
	};
	
	enum ColorSpace
	{
		kColorSpace_Rgb,
		kColorSpace_Rgbw,
		kColorSpace_Hsl
	};

	struct Vector2
	{
		float x = 0.f;
		float y = 0.f;
		
		Vector2()
		{
		}
		
		Vector2(const float in_x, const float in_y)
			: x(in_x)
			, y(in_y)
		{
		}
		
		const float & operator[](const int index) const
		{
			return (&x)[index];
		}
	};
	
	struct Vector3
	{
		float x = 0.f;
		float y = 0.f;
		float z = 0.f;
		
		Vector3()
		{
		}
		
		Vector3(const float in_x, const float in_y, const float in_z)
			: x(in_x)
			, y(in_y)
			, z(in_z)
		{
		}
		
		const float & operator[](const int index) const
		{
			return (&x)[index];
		}
	};
	
	struct Vector4
	{
		float x = 0.f;
		float y = 0.f;
		float z = 0.f;
		float w = 0.f;
		
		Vector4()
		{
		}
		
		Vector4(const float in_x, const float in_y, const float in_z, const float in_w)
			: x(in_x)
			, y(in_y)
			, z(in_z)
			, w(in_w)
		{
		}
	};
	
	struct Color
	{
		ColorSpace colorSpace = kColorSpace_Rgb;
		
		float x = 0.f;
		float y = 0.f;
		float z = 0.f;
		float w = 0.f;
		
		struct Rgb
		{
			float r;
			float g;
			float b;
		};
		
		struct Rgbw
		{
			float r;
			float g;
			float b;
			float w;
		};
		
		struct Hsl
		{
			float hue;
			float saturation;
			float luminance;
		};
		
		void setRgb(const float r, const float g, const float b);
		
		bool operator==(const Color & other) const;
		bool operator!=(const Color & other) const;
	};
	
	struct ColorRgba
	{
		float r = 0.f;
		float g = 0.f;
		float b = 0.f;
		float a = 0.f;

		void set(const float r, const float g, const float b, const float a);
	};
	
	struct Label
	{
		std::string text;
	};

	struct Knob
	{
		std::string name;
		std::string displayName;
		float defaultValue = 0.f;
		bool hasDefaultValue = false;
		float min = 0.f;
		float max = 1.f;
		float exponential = 1.f;
		Unit unit = kUnit_Float;
		std::string oscAddress;
	};
	
	struct Slider2
	{
		std::string name;
		std::string displayName;
		Vector2 defaultValue = { 0.f, 0.f };
		bool hasDefaultValue = false;
		Vector2 min = { 0.f, 0.f };
		Vector2 max = { 1.f, 1.f };
		std::string oscAddress;
	};
	
	struct Slider3
	{
		std::string name;
		std::string displayName;
		Vector3 defaultValue = { 0.f, 0.f, 0.f };
		bool hasDefaultValue = false;
		Vector3 min = { 0.f, 0.f, 0.f };
		Vector3 max = { 1.f, 1.f, 1.f };
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
	
	struct ColorPicker
	{
		std::string name;
		std::string displayName;
		ColorSpace colorSpace = kColorSpace_Rgb;
		Vector4 defaultValue;
		bool hasDefaultValue = false;
		std::string oscAddress;
	};
	
	struct Separator
	{
		ColorRgba borderColor;
		bool hasBorderColor = false;
		int thickness = 1;
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
		
		Slider2 slider2;
		Slider3 slider3;
		
		Listbox listbox;
		
		ColorPicker colorPicker;
		
		Separator separator;
		
		void makeLabel()
		{
			type = kElementType_Label;
			
			sx = 100;
			sy = 16;
		}
		
		void makeKnob()
		{
			type = kElementType_Knob;
			
			sx = 48;
			sy = 48;
		}
		
		void makeSlider2()
		{
			type = kElementType_Slider2;
			
			sx = 100;
			sy = 40;
		}
		
		void makeSlider3()
		{
			type = kElementType_Slider3;
			
			sx = 100;
			sy = 40;
		}
		
		void makeListbox()
		{
			type = kElementType_Listbox;
			
			sx = 100;
			sy = 20;
		}
		
		void makeColorPicker()
		{
			type = kElementType_ColorPicker;
			
			sx = 200;
			sy = 100;
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
		void initializeDisplayNames();
		void performLayout();
	};
	
	void reflect(TypeDB & typeDB);
}

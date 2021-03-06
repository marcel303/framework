#pragma once

#include <string>
#include <vector>

struct StructuredType;
struct TypeDB;

// todo : move layout definition to a separate file
namespace ControlSurfaceDefinition
{
	struct ElementLayout
	{
		std::string groupName;
		std::string name;
		
		bool hasPosition = false;
		int x, y;
		
		bool hasSize = false;
		int sx, sy;
		
		static void reflect(TypeDB & typeDB);
	};
	
	struct SurfaceLayout
	{
		int sx = 0;
		int sy = 0;
		
		int marginX = 0;
		int marginY = 0;
		
		int paddingX = 0;
		int paddingY = 0;
		
		std::vector<ElementLayout> elems;
		
		ElementLayout * addElement(const char * groupName, const char * name);
		ElementLayout * findElement(const char * groupName, const char * name);
		const ElementLayout * findElement(const char * groupName, const char * name) const;
	};
	
	struct LayoutConstraintsBase
	{
		virtual void getElementSizeConstraints(
			const char * groupName,
			const char * name,
			bool & hasMinSize,
			int & minSx,
			int & minSy,
			bool & hasMaxSize,
			int & maxSx,
			int & maxSy) const = 0;
	};
}

namespace ControlSurfaceDefinition
{
	struct Element;
	
	enum ElementType
	{
		kElementType_None,
		kElementType_Label,
		kElementType_Knob,
		kElementType_Button,
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
		
		void set(const float in_x, const float in_y)
		{
			x = in_x;
			y = in_y;
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
		
		void set(const float in_x, const float in_y, const float in_z)
		{
			x = in_x;
			y = in_y;
			z = in_z;
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
		
		bool operator==(const Vector4 & other) const;
		bool operator!=(const Vector4 & other) const;
		
		const float & operator[](const int index) const
		{
			return (&x)[index];
		}
		
		float & operator[](const int index)
		{
			return (&x)[index];
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
		std::string displayName;
		float defaultValue = 0.f;
		bool hasDefaultValue = false;
		float min = 0.f;
		float max = 1.f;
		float exponential = 1.f;
		Unit unit = kUnit_Float;
		std::string oscAddress;
	};
	
	struct Button
	{
		std::string displayName;
		bool defaultValue = false;
		bool hasDefaultValue = false;
		bool isToggle = false;
		std::string oscAddress;
	};
	
	struct Slider2
	{
		std::string displayName;
		Vector2 defaultValue = { 0.f, 0.f };
		bool hasDefaultValue = false;
		Vector2 min = { 0.f, 0.f };
		Vector2 max = { 1.f, 1.f };
		Vector2 exponential = { 1.f, 1.f };
		std::string oscAddress;
	};
	
	struct Slider3
	{
		std::string displayName;
		Vector3 defaultValue = { 0.f, 0.f, 0.f };
		bool hasDefaultValue = false;
		Vector3 min = { 0.f, 0.f, 0.f };
		Vector3 max = { 1.f, 1.f, 1.f };
		Vector3 exponential = { 1.f, 1.f, 1.f };
		std::string oscAddress;
	};
	
	struct Listbox
	{
		std::vector<std::string> items;
		std::string defaultValue;
		bool hasDefaultValue = false;
		std::string oscAddress;
	};
	
	struct ColorPicker
	{
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
		std::string name;
		
		ElementType type = kElementType_None;
		
		int initialSx = 0;
		int initialSy = 0;
		
		bool divideLeft = false;
		bool divideRight = false;
		bool divideBottom = false;
		
		Label label;
		
		Knob knob;
		
		Button button;
		
		Slider2 slider2;
		Slider3 slider3;
		
		Listbox listbox;
		
		ColorPicker colorPicker;
		
		Separator separator;
		
		void makeLabel()
		{
			type = kElementType_Label;
			
			initialSx = 100;
			initialSy = 16;
		}
		
		void makeKnob()
		{
			type = kElementType_Knob;
			
			initialSx = 48;
			initialSy = 48;
		}
		
		void makeButton()
		{
			type = kElementType_Button;
			
			initialSx = 100;
			initialSy = 40;
		}
		
		void makeSlider2()
		{
			type = kElementType_Slider2;
			
			initialSx = 100;
			initialSy = 40;
		}
		
		void makeSlider3()
		{
			type = kElementType_Slider3;
			
			initialSx = 100;
			initialSy = 40;
		}
		
		void makeListbox()
		{
			type = kElementType_Listbox;
			
			initialSx = 100;
			initialSy = 20;
		}
		
		void makeColorPicker()
		{
			type = kElementType_ColorPicker;
			
			initialSx = 120;
			initialSy = 70;
		}
		
		void makeSeparator()
		{
			type = kElementType_Separator;
			
			initialSx = 6;
			initialSy = 6;
			
			divideLeft = true;
			divideRight = true;
		}
	};
	
	struct Group
	{
		std::string name;
		std::vector<Element> elems;
	};
	
	struct Surface : LayoutConstraintsBase
	{
		std::string name;
		
		std::vector<Group> groups;
		
		SurfaceLayout layout;
		
		void initializeNames();
		void initializeDefaultValues();
		void initializeDisplayNames();
		void performLayout();
		
		Element * findElement(const char * groupName, const char * name);
		const Element * findElement(const char * groupName, const char * name) const;
		
		// -- LayoutConstraintsBase
		
		virtual void getElementSizeConstraints(
			const char * groupName,
			const char * name,
			bool & hasMinSize,
			int & minSx,
			int & minSy,
			bool & hasMaxSize,
			int & maxSx,
			int & maxSy) const override;
	};
	
	void reflect(TypeDB & typeDB);
}

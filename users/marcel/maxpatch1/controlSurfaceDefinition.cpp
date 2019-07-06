#include "controlSurfaceDefinition.h"
#include "reflection.h"
#include <assert.h>

namespace ControlSurfaceDefinition
{
	bool Vector4::operator==(const Vector4 & other) const
	{
		return
			x == other.x &&
			y == other.y &&
			z == other.z &&
			w == other.w;
	}
	
	bool Vector4::operator!=(const Vector4 & other) const
	{
		return (*this == other) == false;
	}
	
	//
	
	void Color::setRgb(
		const float in_r,
		const float in_g,
		const float in_b)
	{
		colorSpace = kColorSpace_Rgb;
		
		x = in_r;
		y = in_g;
		z = in_b;
		w = 0.f;
	}

	bool Color::operator==(const Color & other) const
	{
		if (colorSpace != other.colorSpace)
		{
			return false;
		}
		
		if (colorSpace == kColorSpace_Rgb)
		{
			return
				x == other.x &&
				y == other.y &&
				z == other.z;
		}
		else if (colorSpace == kColorSpace_Rgbw)
		{
			return
				x == other.x &&
				y == other.y &&
				z == other.z &&
				w == other.w;
		}
		else if (colorSpace == kColorSpace_Hsl)
		{
			return
				x == other.x &&
				y == other.y &&
				z == other.z;
		}
		else
		{
			assert(false);
			return false;
		}
	}
	
	bool Color::operator!=(const Color & other) const
	{
		return (*this == other) == false;
	}

	//
	
	void ColorRgba::set(
		const float in_r,
		const float in_g,
		const float in_b,
		const float in_a)
	{
		r = in_r;
		g = in_g;
		b = in_b;
		a = in_a;
	}
	
	//
	
	void Surface::initializeDefaultValues()
	{
		for (auto & group : groups)
		{
			for (auto & elem : group.elems)
			{
				if (elem.type == kElementType_Knob)
				{
					auto & knob = elem.knob;
					
					if (knob.hasDefaultValue == false)
					{
						knob.defaultValue = knob.min;
						knob.hasDefaultValue = true;
					}
				}
			}
		}
	}
	
	void Surface::initializeDisplayNames()
	{
		for (auto & group : groups)
		{
			for (auto & elem : group.elems)
			{
				if (elem.type == kElementType_Knob)
				{
					auto & knob = elem.knob;
					
					if (knob.displayName.empty())
						knob.displayName = knob.name;
				}
			}
		}
	}
	
	void Surface::performLayout()
	{
		int x = layout.marginX;
		int y = layout.marginY;
		
		for (auto & group : groups)
		{
			int initialY = layout.marginY;
			
			int sx = 0;
			
			for (auto & elem : group.elems)
			{
				if (elem.divideLeft)
				{
					x += sx;
					y = initialY;
					
					sx = 0;
				}
				
				if (elem.type == kElementType_Separator)
					elem.sy = layout.sy - initialY - layout.marginY;
				
				const bool fits = y + elem.sy + layout.paddingY <= layout.sy - layout.marginY;
				
				if (fits == false)
				{
					x += sx;
					y = initialY;
					
					sx = 0;
				}
				
				elem.x = x;
				elem.y = y;
				
				y += elem.sy + layout.paddingY;
				
				if (elem.sx > sx)
					sx = elem.sx + layout.paddingX;
				
				if (elem.divideRight)
				{
					x += sx;
					y = initialY;
					
					sx = 0;
				}
				
				if (elem.divideBottom)
				{
					initialY = y;
					
					sx = 0;
				}
			}
			
			// end the current group
			
			x += sx;
			y = layout.marginY;
			
			sx = 0;
		}
	}
	
	//
	
	void reflect(TypeDB & typeDB)
	{
		typeDB.addEnum<ElementType>("ControlSurfaceDefinition::ElementType")
			.add("none", kElementType_None)
			.add("label", kElementType_Label)
			.add("knob", kElementType_Knob)
			.add("slider2", kElementType_Slider2)
			.add("slider3", kElementType_Slider3)
			.add("listbox", kElementType_Listbox)
			.add("colorPicker", kElementType_ColorPicker)
			.add("separator", kElementType_Separator);

		typeDB.addEnum<Unit>("ControlSurfaceDefinition::Unit")
			.add("int", kUnit_Int)
			.add("float", kUnit_Float)
			.add("time", kUnit_Time)
			.add("hertz", kUnit_Hertz)
			.add("decibel", kUnit_Decibel)
			.add("percentage", kUnit_Percentage);
		
		typeDB.addEnum<ColorSpace>("ControlSurfaceDefinition::ColorSpace")
			.add("rgb", kColorSpace_Rgb)
			.add("rgbw", kColorSpace_Rgbw)
			.add("hsl", kColorSpace_Hsl);
		
		typeDB.addStructured<Vector2>("ControlSurfaceDefinition::Vector2")
			.add("x", &Vector2::x)
			.add("y", &Vector2::y);
		
		typeDB.addStructured<Vector3>("ControlSurfaceDefinition::Vector3")
			.add("x", &Vector3::x)
			.add("y", &Vector3::y)
			.add("z", &Vector3::z);
		
		typeDB.addStructured<Vector4>("ControlSurfaceDefinition::Vector4")
			.add("x", &Vector4::x)
			.add("y", &Vector4::y)
			.add("z", &Vector4::z)
			.add("w", &Vector4::w);
		
		typeDB.addStructured<Color>("ControlSurfaceDefinition::Color")
			.add("colorSpace", &Color::colorSpace)
			.add("x", &Color::x)
			.add("y", &Color::y)
			.add("z", &Color::z)
			.add("w", &Color::w);
		
		typeDB.addPlain<ColorRgba>("ControlSurfaceDefinition::ColorRgba", kDataType_Float4);
		
		typeDB.addStructured<Label>("ControlSurfaceDefinition::Label")
			.add("text", &Label::text);
			
		typeDB.addStructured<Knob>("ControlSurfaceDefinition::Knob")
			.add("name", &Knob::name)
			.add("defaultValue", &Knob::defaultValue)
			.add("hasDefaultValue", &Knob::hasDefaultValue)
			.add("min", &Knob::min)
			.add("max", &Knob::max)
			.add("exponential", &Knob::exponential)
			.add("unit", &Knob::unit)
			.add("oscAddress", &Knob::oscAddress);
		
		typeDB.addStructured<Slider2>("ControlSurfaceDefinition::Slider2")
			.add("name", &Slider2::name)
			.add("defaultValue", &Slider2::defaultValue)
			.add("hasDefaultValue", &Slider2::hasDefaultValue)
			.add("min", &Slider2::min)
			.add("max", &Slider2::max)
			.add("oscAddress", &Slider2::oscAddress);
		
		typeDB.addStructured<Slider3>("ControlSurfaceDefinition::Slider3")
			.add("name", &Slider3::name)
			.add("defaultValue", &Slider3::defaultValue)
			.add("hasDefaultValue", &Slider3::hasDefaultValue)
			.add("min", &Slider3::min)
			.add("max", &Slider3::max)
			.add("oscAddress", &Slider3::oscAddress);
		
		typeDB.addStructured<Listbox>("ControlSurfaceDefinition::Listbox")
			.add("name", &Listbox::name)
			.add("items", &Listbox::items)
			.add("defaultValue", &Listbox::defaultValue)
			.add("hasDefaultValue", &Listbox::hasDefaultValue)
			.add("oscAddress", &Listbox::oscAddress);
		
		typeDB.addStructured<ColorPicker>("ControlSurfaceDefinition::ColorPicker")
			.add("name", &ColorPicker::name)
			.add("displayName", &ColorPicker::displayName)
			.add("defaultValue", &ColorPicker::defaultValue)
			.add("hasDefaultValue", &ColorPicker::hasDefaultValue)
			.add("oscAddress", &ColorPicker::oscAddress);
		
		typeDB.addStructured<Separator>("ControlSurfaceDefinition::Separator")
			.add("borderColor", &Separator::borderColor)
			.add("hasBorderColor", &Separator::hasBorderColor)
			.add("thickness", &Separator::thickness);
		
		typeDB.addStructured<Element>("ControlSurfaceDefinition::Element")
			.add("type", &Element::type)
			.add("x", &Element::x)
			.add("y", &Element::y)
			.add("width", &Element::sx)
			.add("height", &Element::sy)
			.add("label", &Element::label)
			.add("knob", &Element::knob)
			.add("slider2", &Element::slider2)
			.add("slider3", &Element::slider3)
			.add("listbox", &Element::listbox)
			.add("colorPicker", &Element::colorPicker)
			.add("separator", &Element::separator);
		
		typeDB.addStructured<Group>("ControlSurfaceDefinition::Group")
			.add("name", &Group::name)
			.add("elements", &Group::elems);
	
		typeDB.addStructured<SurfaceLayout>("ControlSurfaceDefinition::SurfaceLayout")
			.add("sx", &SurfaceLayout::sx)
			.add("sy", &SurfaceLayout::sy)
			.add("marginX", &SurfaceLayout::marginX)
			.add("marginY", &SurfaceLayout::marginY)
			.add("paddingX", &SurfaceLayout::paddingX)
			.add("paddingY", &SurfaceLayout::paddingY);
		
		typeDB.addStructured<Surface>("ControlSurfaceDefinition::Surface")
			.add("groups", &Surface::groups)
			.add("layout", &Surface::layout);
	}
}

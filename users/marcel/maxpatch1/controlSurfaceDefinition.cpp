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
	
	ElementLayout * SurfaceLayout::addElement(const char * groupName, const char * name)
	{
		elems.push_back(ElementLayout());
		auto & elem = elems.back();
		elem.groupName = groupName;
		elem.name = name;
		return &elem;
	}
	
	ElementLayout * SurfaceLayout::findElement(const char * groupName, const char * name)
	{
		for (auto & elem : elems)
			if (elem.groupName == groupName && elem.name == name)
				return &elem;
		
		return nullptr;
	}
	
	const ElementLayout * SurfaceLayout::findElement(const char * groupName, const char * name) const
	{
		return const_cast<SurfaceLayout*>(this)->findElement(groupName, name);
	}
	
	//
	
	void Surface::initializeNames()
	{
		int nextId = 1;
		
		for (auto & group : groups)
		{
			for (auto & elem : group.elems)
			{
				if (elem.name.empty())
				{
					char name[64];
					sprintf(name, "(noname-%d)", nextId++);
					
					elem.name = name;
				}
			}
		}
	}
	
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
				else if (elem.type == kElementType_Slider2)
				{
					auto & slider = elem.slider2;
					
					if (slider.hasDefaultValue == false)
					{
						slider.defaultValue = slider.min;
						slider.hasDefaultValue = true;
					}
				}
				else if (elem.type == kElementType_Slider3)
				{
					auto & slider = elem.slider3;
					
					if (slider.hasDefaultValue == false)
					{
						slider.defaultValue = slider.min;
						slider.hasDefaultValue = true;
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
						knob.displayName = elem.name;
				}
				else if (elem.type == kElementType_Button)
				{
					auto & button = elem.button;
					
					if (button.displayName.empty())
						button.displayName = elem.name;
				}
				else if (elem.type == kElementType_Slider2)
				{
					auto & slider = elem.slider2;
					
					if (slider.displayName.empty())
						slider.displayName = elem.name;
				}
				else if (elem.type == kElementType_Slider3)
				{
					auto & slider = elem.slider3;
					
					if (slider.displayName.empty())
						slider.displayName = elem.name;
				}
				else if (elem.type == kElementType_ColorPicker)
				{
					auto & colorPicker = elem.colorPicker;
					
					if (colorPicker.displayName.empty())
						colorPicker.displayName = elem.name;
				}
			}
		}
	}
	
	void Surface::performLayout()
	{
		layout.elems.clear();
		
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
				
				layout.elems.resize(layout.elems.size() + 1);
				
				auto & elemLayout = layout.elems.back();
				
				elemLayout.groupName = group.name;
				elemLayout.name = elem.name;
				
				elemLayout.hasSize = true;
				elemLayout.sx = elem.initialSx;
				elemLayout.sy = elem.initialSy;
				
				if (elem.type == kElementType_Separator)
					elemLayout.sy = layout.sy - initialY - layout.marginY;
				
				const bool fits = y + elemLayout.sy + layout.paddingY <= layout.sy - layout.marginY;
				
				if (fits == false)
				{
					x += sx;
					y = initialY;
					
					sx = 0;
				}
				
				elemLayout.hasPosition = true;
				elemLayout.x = x;
				elemLayout.y = y;
				
				y += elemLayout.sy + layout.paddingY;
				
				if (elemLayout.sx > sx)
					sx = elemLayout.sx + layout.paddingX;
				
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
	
	Element * Surface::findElement(const char * groupName, const char * name)
	{
		for (auto & group : groups)
			if (group.name == groupName)
				for (auto & elem : group.elems)
					if (elem.name == name)
						return &elem;
		
		return nullptr;
	}
	
	const Element * Surface::findElement(const char * groupName, const char * name) const
	{
		return const_cast<Surface*>(this)->findElement(groupName, name);
	}
	
	//
	
	void reflect(TypeDB & typeDB)
	{
		typeDB.addEnum<ElementType>("ControlSurfaceDefinition::ElementType")
			.add("none", kElementType_None)
			.add("label", kElementType_Label)
			.add("knob", kElementType_Knob)
			.add("button", kElementType_Button)
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
			.add("displayName", &Knob::displayName)
			.add("defaultValue", &Knob::defaultValue)
			.add("hasDefaultValue", &Knob::hasDefaultValue)
			.add("min", &Knob::min)
			.add("max", &Knob::max)
			.add("exponential", &Knob::exponential)
			.add("unit", &Knob::unit)
			.add("oscAddress", &Knob::oscAddress);
		
		typeDB.addStructured<Button>("ControlSurfaceDefinition::Button")
			.add("displayName", &Button::displayName)
			.add("defaultValue", &Button::defaultValue)
			.add("hasDefaultValue", &Button::hasDefaultValue)
			.add("isToggle", &Button::isToggle)
			.add("oscAddress", &Button::oscAddress);
		
		typeDB.addStructured<Slider2>("ControlSurfaceDefinition::Slider2")
			.add("displayName", &Slider2::displayName)
			.add("defaultValue", &Slider2::defaultValue)
			.add("hasDefaultValue", &Slider2::hasDefaultValue)
			.add("min", &Slider2::min)
			.add("max", &Slider2::max)
			.add("exponential", &Slider2::exponential)
			.add("oscAddress", &Slider2::oscAddress);
		
		typeDB.addStructured<Slider3>("ControlSurfaceDefinition::Slider3")
			.add("displayName", &Slider3::displayName)
			.add("defaultValue", &Slider3::defaultValue)
			.add("hasDefaultValue", &Slider3::hasDefaultValue)
			.add("min", &Slider3::min)
			.add("max", &Slider3::max)
			.add("exponential", &Slider3::exponential)
			.add("oscAddress", &Slider3::oscAddress);
		
		typeDB.addStructured<Listbox>("ControlSurfaceDefinition::Listbox")
			.add("items", &Listbox::items)
			.add("defaultValue", &Listbox::defaultValue)
			.add("hasDefaultValue", &Listbox::hasDefaultValue)
			.add("oscAddress", &Listbox::oscAddress);
		
		typeDB.addStructured<ColorPicker>("ControlSurfaceDefinition::ColorPicker")
			.add("displayName", &ColorPicker::displayName)
			.add("colorSpace", &ColorPicker::colorSpace)
			.add("defaultValue", &ColorPicker::defaultValue)
			.add("hasDefaultValue", &ColorPicker::hasDefaultValue)
			.add("oscAddress", &ColorPicker::oscAddress);
		
		typeDB.addStructured<Separator>("ControlSurfaceDefinition::Separator")
			.add("borderColor", &Separator::borderColor)
			.add("hasBorderColor", &Separator::hasBorderColor)
			.add("thickness", &Separator::thickness);
		
		typeDB.addStructured<Element>("ControlSurfaceDefinition::Element")
			.add("name", &Element::name)
			.add("type", &Element::type)
			.add("width", &Element::initialSx)
			.add("height", &Element::initialSy)
			.add("divideLeft", &Element::divideLeft)
			.add("divideRight", &Element::divideRight)
			.add("divideBottom", &Element::divideBottom)
			.add("label", &Element::label)
			.add("knob", &Element::knob)
			.add("button", &Element::button)
			.add("slider2", &Element::slider2)
			.add("slider3", &Element::slider3)
			.add("listbox", &Element::listbox)
			.add("colorPicker", &Element::colorPicker)
			.add("separator", &Element::separator);
		
		typeDB.addStructured<ElementLayout>("ControlSurfaceDefinition::ElementLayout")
			.add("groupName", &ElementLayout::groupName)
			.add("name", &ElementLayout::name)
			.add("hasPosition", &ElementLayout::hasPosition)
			.add("x", &ElementLayout::x)
			.add("y", &ElementLayout::y)
			.add("hasSize", &ElementLayout::hasSize)
			.add("width", &ElementLayout::sx)
			.add("height", &ElementLayout::sy);
		
		typeDB.addStructured<Group>("ControlSurfaceDefinition::Group")
			.add("name", &Group::name)
			.add("elements", &Group::elems);
	
		typeDB.addStructured<SurfaceLayout>("ControlSurfaceDefinition::SurfaceLayout")
			.add("width", &SurfaceLayout::sx)
			.add("height", &SurfaceLayout::sy)
			.add("marginX", &SurfaceLayout::marginX)
			.add("marginY", &SurfaceLayout::marginY)
			.add("paddingX", &SurfaceLayout::paddingX)
			.add("paddingY", &SurfaceLayout::paddingY)
			.add("elements", &SurfaceLayout::elems);
		
		typeDB.addStructured<Surface>("ControlSurfaceDefinition::Surface")
			.add("name", &Surface::name)
			.add("groups", &Surface::groups)
			.add("layout", &Surface::layout);
	}
}

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

#include "framework.h"
#include "jsusfx-framework.h"

JsusFx_Framework::JsusFx_Framework(JsusFxPathLibrary &pathLibrary)
	: JsusFx(pathLibrary)
{
}

void JsusFx_Framework::displayMsg(const char *fmt, ...)
{
    char output[4096];
    va_list argptr;
    va_start(argptr, fmt);
    vsnprintf(output, 4095, fmt, argptr);
    va_end(argptr);

    logDebug(output);
}

void JsusFx_Framework::displayError(const char *fmt, ...)
{
    char output[4096];
    va_list argptr;
    va_start(argptr, fmt);
    vsnprintf(output, 4095, fmt, argptr);
    va_end(argptr);

    logError(output);
}

//

#include "jsusfx_serialize.h"
#include "tinyxml2.h"
#include "tinyxml2_helpers.h"

void saveJsusFxSerializationDataToXml(const JsusFxSerializationData & serializationData, tinyxml2::XMLPrinter & p)
{
	if (!serializationData.sliders.empty())
	{
		p.OpenElement("sliders");
		{
			for (auto & slider : serializationData.sliders)
			{
				p.OpenElement("slider");
				{
					p.PushAttribute("index", slider.index);
					p.PushAttribute("value", slider.value);
				}
				p.CloseElement();
			}
		}
		p.CloseElement();
	}

	if (!serializationData.vars.empty())
	{
		p.OpenElement("vars");
		{
			for (auto & var : serializationData.vars)
			{
				p.OpenElement("var");
				{
					p.PushAttribute("value", var);
				}
				p.CloseElement();
			}
		}
		p.CloseElement();
	}
}

bool loadJsusFxSerializationDataFromXml(const tinyxml2::XMLElement * e, JsusFxSerializationData & serializationData)
{
	auto xml_sliders = e->FirstChildElement("sliders");

	if (xml_sliders != nullptr)
	{
		for (auto xml_slider = xml_sliders->FirstChildElement("slider"); xml_slider != nullptr; xml_slider = xml_slider->NextSiblingElement("slider"))
		{
			JsusFxSerializationData::Slider slider;
			
			slider.index = intAttrib(xml_slider, "index", -1);
			slider.value = floatAttrib(xml_slider, "value", 0.f);
			
			serializationData.sliders.push_back(slider);
		}
	}

	auto xml_vars = e->FirstChildElement("vars");

	if (xml_vars != nullptr)
	{
		for (auto xml_var = xml_vars->FirstChildElement("var"); xml_var != nullptr; xml_var = xml_var->NextSiblingElement("var"))
		{
			const float value = floatAttrib(xml_var, "value", 0.f);
			
			serializationData.vars.push_back(value);
		}
	}
	
	return true;
}

#include "maxPatchEditor.h"

namespace max
{
	BoxEditor PatchEditor::beginBox(const char * id, const int numInlets, const int numOutlets)
	{
		patch.patcher.boxes.resize(patch.patcher.boxes.size() + 1);
		auto & box = patch.patcher.boxes.back();
		box.box.id = id;
		box.box.numinlets = numInlets;
		box.box.numoutlets = numOutlets;
		
		return BoxEditor(*this, box.box);
	}
	
	BoxEditor & BoxEditor::comment(const char * comment)
	{
		box.comment = comment;
		return *this;
	}
	
	BoxEditor & BoxEditor::maxclass(const char * maxclass)
	{
		box.maxclass = maxclass;
		return *this;
	}
	
	BoxEditor & BoxEditor::patching_rect(const int x, const int y, const int width, const int height)
	{
		box.patching_rect = { x, y, width, height };
		return *this;
	}
	
	BoxEditor & BoxEditor::presentation(const bool presentation)
	{
		box.presentation = presentation;
		return *this;
	}
	
	BoxEditor & BoxEditor::presentation_rect(const int x, const int y, const int width, const int height)
	{
		box.presentation_rect = { x, y, width, height };
		return *this;
	}
	
	BoxEditor & BoxEditor::text(const char * text)
	{
		box.text = text;
		return *this;
	}
	
	BoxEditor & BoxEditor::linecolor(const float r, const float g, const float b, const float a)
	{
		box.linecolor = { r, g, b, a };
		return *this;
	}
	
	BoxEditor & BoxEditor::border(const int border)
	{
		box.border = border;
		return *this;
	}
	
	BoxEditor & BoxEditor::saved_attribute_attributes(const std::vector<SavedAttribute> & saved_attribute_attributes)
	{
		box.saved_attribute_attributes.valueof = saved_attribute_attributes;
		return *this;
	}
	
	BoxEditor & BoxEditor::saved_attribute(const char * name, const int value)
	{
		box.saved_attribute_attributes.valueof.push_back(SavedAttribute(name, value));
		return *this;
	}
	
	BoxEditor & BoxEditor::saved_attribute(const char * name, const float value)
	{
		box.saved_attribute_attributes.valueof.push_back(SavedAttribute(name, value));
		return *this;
	}
	
	BoxEditor & BoxEditor::saved_attribute(const char * name, const std::string & value)
	{
		box.saved_attribute_attributes.valueof.push_back(SavedAttribute(name, value));
		return *this;
	}
	
	BoxEditor & BoxEditor::saved_attribute(const char * name, const std::vector<int> & value)
	{
		box.saved_attribute_attributes.valueof.push_back(SavedAttribute(name, value));
		return *this;
	}
	
	BoxEditor & BoxEditor::saved_attribute(const char * name, const std::vector<float> & value)
	{
		box.saved_attribute_attributes.valueof.push_back(SavedAttribute(name, value));
		return *this;
	}
	
	BoxEditor & BoxEditor::saved_attribute(const char * name, const std::vector<std::string> & value)
	{
		box.saved_attribute_attributes.valueof.push_back(SavedAttribute(name, value));
		return *this;
	}

	BoxEditor & BoxEditor::parameter_enable(const bool parameter_enable)
	{
		box.parameter_enable = parameter_enable;
		return *this;
	}
	
	BoxEditor & BoxEditor::varname(const char * varname)
	{
		box.varname = varname;
		return *this;
	}
	
	PatchEditor & BoxEditor::end()
	{
		return patchEditor;
	}
}

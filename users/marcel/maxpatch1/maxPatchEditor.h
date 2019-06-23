#pragma once

#include "maxPatch.h"
#include <string>
#include <vector>

namespace max
{
	struct BoxEditor;
	struct PatchEditor;
	
	struct PatchEditor
	{
		Patch & patch;
		
		PatchEditor(Patch & in_patch)
			: patch(in_patch)
		{
		}
		
		BoxEditor beginBox(const char * id, const int numInlets, const int numOutlets);
		
		void connect(const char * sourceBoxId, const int outletIndex, const char * destinationBoxId, const int inletIndex);
		void beginConnection();
	};
	
	struct BoxEditor
	{
		PatchEditor & patchEditor;
		Box & box;
		
		BoxEditor(PatchEditor & in_patchEditor, Box & in_box)
			: patchEditor(in_patchEditor)
			, box(in_box)
		{
		}
		
		BoxEditor & comment(const char * comment);
		BoxEditor & maxclass(const char * maxclass);
		//std::vector<std::string> outlettype;
		BoxEditor & patching_rect(const int x, const int y, const int width, const int height);
		BoxEditor & presentation(const bool presentation);
		BoxEditor & presentation_rect(const int x, const int y, const int width, const int height);
		BoxEditor & text(const char * text);
		BoxEditor & linecolor(const float r, const float g, const float b, const float a);
		BoxEditor & border(const int border);
		
		BoxEditor & saved_attribute_attributes(const std::vector<SavedAttribute> & saved_attribute_attributes);
		BoxEditor & saved_attribute(const char * name, const int value);
		BoxEditor & saved_attribute(const char * name, const float value);
		BoxEditor & saved_attribute(const char * name, const std::string & value);
		BoxEditor & saved_attribute(const char * name, const std::vector<int> & value);
		BoxEditor & saved_attribute(const char * name, const std::vector<float> & value);
		BoxEditor & saved_attribute(const char * name, const std::vector<std::string> & value);
		BoxEditor & parameter_enable(const bool parameter_enable);
		BoxEditor & varname(const char * varname);
		
		PatchEditor & end();
	};
}

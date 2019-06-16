#pragma once

#include <string>
#include <vector>

struct TypeDB;

namespace max
{
	struct AppVersion
	{
		int major = 7;
		int minor = 2;
		int revision = 0;
		
		static void reflect(TypeDB & typeDB);
	};
	
	struct SavedAttribute
	{
		std::string name;
		std::string value;
	};
	
	struct SavedAttributeAttributes
	{
		std::vector<SavedAttribute> valueof;
		
		static void reflect(TypeDB & typeDB);
	};

	struct Box
	{
		std::string comment;
		std::string id; // unique id
		std::string maxclass = "newobj";
		int numinlets = 0;
		int numoutlets = 0;
		std::vector<std::string> outlettype;
		std::vector<int> patching_rect = { 0, 0, 10, 10 };
		bool presentation = false;
		std::vector<int> presentation_rect = { 0, 0, 10, 10 };
		std::string text; // this contains the arguments passed to the node
		
		// this member requires custom serialization, since Max stores it rather oddly.. not as structured data
		SavedAttributeAttributes saved_attribute_attributes;
		bool parameter_enable = false;
		std::string varname;
		
		// todo : a box may contain a sub-patcher (Patcher patcher)
		
		static void reflect(TypeDB & typeDB);
	};
	
	struct Line
	{
		std::vector<std::string> destination = { "", "0" }; // name and index
		bool disabled = false;
		bool hidden = false;
		std::vector<std::string> source = { "", "0" }; // name and index
		
		static void reflect(TypeDB & typeDB);
	};
	
	// --- Patcher ---
	
	struct PatchBox
	{
		Box box;
		
		static void reflect(TypeDB & typeDB);
	};
	
	struct PatchLine
	{
		Line patchline;
		
		static void reflect(TypeDB & typeDB);
	};
	
	struct Patcher
	{
		int fileversion = 1;
		AppVersion appversion;
		std::vector<float> rect = { 0, 0, 800, 600 };
		bool openinpresentation = true;
		float default_fontsize = 12.f;
		int default_fontface = 0;
		std::string default_fontname = "Andale Mono";
		std::string description;
		std::vector<PatchBox> boxes;
		std::vector<PatchLine> lines;
		
		static void reflect(TypeDB & typeDB);
	};
	
	struct Patch
	{
		Patcher patcher;
		
		static void reflect(TypeDB & typeDB);
	};
}

namespace max
{
	struct BoxEditor;
	
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
		
		BoxEditor & saved_attribute_attributes(const std::vector<SavedAttribute> & saved_attribute_attributes);
		BoxEditor & parameter_enable(const bool parameter_enable);
		BoxEditor & varname(const char * varname);
		
		PatchEditor & end();
	};
}

#pragma once

#include <string>
#include <vector>

struct TypeDB;

namespace max
{
	enum ParameterType // todo : reflection, make member
	{
		kParameterType_Float,
		kParameterType_Uint8,
		kParameterType_Enum,
		kParameterType_Blob
	};

// applies to live.dial, live.numbox
	enum UnitStyle // todo : reflection, make member
	{
		kUnitStyle_Int,
		kUnitStyle_Float,
		kUnitStyle_Time,
		kUnitStyle_Hertz,
		kUnitStyle_Decibel,
		kUnitStyle_Percentage
	};
	
	struct AppVersion
	{
		int major = 7;
		int minor = 2;
		int revision = 0;
		
		static void reflect(TypeDB & typeDB);
	};
	
	struct SavedAttribute
	{
		enum Type
		{
			kType_None,
			
			kType_Int,
			kType_Float,
			kType_String,
			
			kType_IntArray,
			kType_FloatArray,
			kType_StringArray,
		};
		
		Type type = kType_None;
		std::string name;
		
		int intValue = 0;
		float floatValue = 0.f;
		std::string stringValue;
		
		std::vector<int> intArrayValue;
		std::vector<float> floatArrayValue;
		std::vector<std::string> stringArrayValue;
		
		SavedAttribute() { }
		
		explicit SavedAttribute(const char * in_name, const int value) { type = kType_Int; name = in_name; intValue = value; }
		explicit SavedAttribute(const char * in_name, const float value) { type = kType_Float; name = in_name; floatValue = value; }
		explicit SavedAttribute(const char * in_name, const char * value) { type = kType_String; name = in_name; stringValue = value; }
		explicit SavedAttribute(const char * in_name, const std::string & value) { type = kType_String; name = in_name; stringValue = value; }
		explicit SavedAttribute(const char * in_name, const std::vector<int> & value) { type = kType_IntArray; name = in_name; intArrayValue = value; }
		explicit SavedAttribute(const char * in_name, const std::vector<float> & value) { type = kType_FloatArray; name = in_name; floatArrayValue = value; }
		explicit SavedAttribute(const char * in_name, const std::vector<std::string> & value) { type = kType_StringArray; name = in_name; stringArrayValue = value; }
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
		int mode = 0;
		std::vector<float> linecolor = { 0, 0, 0, 1 };
		int border = 1;
		
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
		float default_fontsize = 10.f;
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
	
	void reflect(TypeDB & typeDB);
}

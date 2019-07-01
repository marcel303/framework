#include "maxPatch.h"
#include "reflection.h"
#include "reflection-jsonio.h"

namespace max
{
	void AppVersion::reflect(TypeDB & typeDB)
	{
		typeDB.addStructured<max::AppVersion>("max::AppVersion")
			.add("major", &AppVersion::major)
			.add("minor", &AppVersion::minor)
			.add("revision", &AppVersion::revision);
	}

	void SavedAttributeAttributes::reflect(TypeDB & typeDB)
	{
	// todo : custom json serialization flag should be set here
		typeDB.addStructured<max::SavedAttributeAttributes>("max::SavedAttributeAttributes");
	}

	static bool savedAttributeAttributesToJson(const TypeDB & typeDB, const Member * member, const void * member_object, REFLECTIONIO_JSON_WRITER & writer)
	{
		SavedAttributeAttributes * attrs = (SavedAttributeAttributes*)member_object;
		
		writer.StartObject();
		{
			writer.Key("valueof");
			writer.StartObject();
			{
				for (auto & attr : attrs->valueof)
				{
					writer.Key(attr.name.c_str());
					
					if (attr.type == SavedAttribute::kType_Int)
						writer.Int(attr.intValue);
					else if (attr.type == SavedAttribute::kType_Float)
						writer.Double(attr.floatValue);
					else if (attr.type == SavedAttribute::kType_String)
						writer.String(attr.stringValue.c_str());
					
					if (attr.type == SavedAttribute::kType_IntArray)
					{
						writer.StartArray();
						{
							for (auto & value : attr.intArrayValue)
								writer.Int(value);
						}
						writer.EndArray();
					}
					else if (attr.type == SavedAttribute::kType_FloatArray)
					{
						writer.StartArray();
						{
							for (auto & value : attr.floatArrayValue)
								writer.Double(value);
						}
						writer.EndArray();
					}
					else if (attr.type == SavedAttribute::kType_StringArray)
					{
						writer.StartArray();
						{
							for (auto & value : attr.stringArrayValue)
								writer.String(value.c_str());
						}
						writer.EndArray();
					}
					
				}
			}
			writer.EndObject();
		}
		writer.EndObject();
		
		return true;
	}

	//

	void Box::reflect(TypeDB & typeDB)
	{
		typeDB.addStructured<max::Box>("max::Box")
			.add("comment", &Box::comment)
			.add("id", &Box::id)
			.add("maxclass", &Box::maxclass)
			.add("numinlets", &Box::numinlets)
			.add("numoutlets", &Box::numoutlets)
			.add("outlettype", &Box::outlettype)
			.add("patching_rect", &Box::patching_rect)
			.add("presentation", &Box::presentation)
			.add("presentation_rect", &Box::presentation_rect)
			.add("text", &Box::text)
			.add("linecolor", &Box::linecolor)
			.add("border", &Box::border)
			.add("saved_attribute_attributes", &Box::saved_attribute_attributes)
				.addFlag(customJsonSerializationFlag(savedAttributeAttributesToJson))
			.add("parameter_enable", &Box::parameter_enable)
			.add("varname", &Box::varname);
	}

	void Line::reflect(TypeDB & typeDB)
	{
		typeDB.addStructured<max::Line>("max::Line")
			.add("destination", &Line::destination)
			.add("disabled", &Line::disabled)
			.add("hidden", &Line::hidden)
			.add("source", &Line::source);
	}

	// --- Patcher ---

	void PatchBox::reflect(TypeDB & typeDB)
	{
		typeDB.addStructured<max::PatchBox>("max::PatchBox")
			.add("box", &PatchBox::box);
	}

	void PatchLine::reflect(TypeDB & typeDB)
	{
		typeDB.addStructured<max::PatchLine>("max::PatchLine")
			.add("patchline", &PatchLine::patchline);
	}

	//

	void Patcher::reflect(TypeDB & typeDB)
	{
		typeDB.addStructured<max::Patcher>("max::Patcher")
			.add("fileversion", &Patcher::fileversion)
			.add("appversion", &Patcher::appversion)
			.add("rect", &Patcher::rect)
			.add("openinpresentation", &Patcher::openinpresentation)
			.add("default_fontsize", &Patcher::default_fontsize)
			.add("default_fontface", &Patcher::default_fontface)
			.add("default_fontname", &Patcher::default_fontname)
			.add("description", &Patcher::description)
			.add("boxes", &Patcher::boxes)
			.add("lines", &Patcher::lines);
	}

	//

	void Patch::reflect(TypeDB & typeDB)
	{
		typeDB.addStructured<max::Patch>("max::Patch")
			.add("patcher", &Patch::patcher);
	}
	
	//
	
	void reflect(TypeDB & typeDB)
	{
		AppVersion::reflect(typeDB);
		SavedAttributeAttributes::reflect(typeDB);
		Box::reflect(typeDB);
		Line::reflect(typeDB);
		PatchBox::reflect(typeDB);
		PatchLine::reflect(typeDB);
		Patcher::reflect(typeDB);
		Patch::reflect(typeDB);
	}
}

#include "maxPatchGenerator.h"
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
					writer.String(attr.value.c_str());
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
	
	// --- editing ---
	
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
	
	BoxEditor & BoxEditor::saved_attribute_attributes(const std::vector<SavedAttribute> & saved_attribute_attributes)
	{
		box.saved_attribute_attributes.valueof = saved_attribute_attributes;
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

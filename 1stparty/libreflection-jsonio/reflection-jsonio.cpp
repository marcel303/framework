#include "Debugging.h"
#include "Log.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "reflection.h"
#include "reflection-jsonio.h"
#include "Vec2.h"
#include "Vec3.h"
#include "Vec4.h"
#include <string>

bool plain_type_fromjson(const PlainType * plain_type, void * object, const rapidjson::Document::ValueType & json)
{
	switch (plain_type->dataType)
	{
	case kDataType_Bool:
		if (json.IsBool() == false)
			return false;
		plain_type->access<bool>(object) = json.GetBool();
		return true;
		
	case kDataType_Int:
		if (json.IsInt() == false)
			return false;
		plain_type->access<int>(object) = json.GetInt();
		return true;

	case kDataType_Float:
		if (json.IsFloat() == false)
			return false;
		plain_type->access<float>(object) = json.GetFloat();
		return true;
		
	case kDataType_Float2:
		{
			if (json.IsObject() == false)
				return false;
			
			auto & value = plain_type->access<Vec2>(object);
			
			auto object = json.GetObject();
			
			for (auto member = object.begin(); member != object.end(); ++member)
			{
				if (member->name == "x")
					value[0] = member->value.GetFloat();
				else if (member->name == "y")
					value[1] = member->value.GetFloat();
				else
					return false;
			}
		}
		return true;
		
	case kDataType_Float3:
		{
			if (json.IsObject() == false)
				return false;
			
			auto & value = plain_type->access<Vec3>(object);
			
			auto object = json.GetObject();
			
			for (auto member = object.begin(); member != object.end(); ++member)
			{
				if (member->name == "x")
					value[0] = member->value.GetFloat();
				else if (member->name == "y")
					value[1] = member->value.GetFloat();
				else if (member->name == "z")
					value[2] = member->value.GetFloat();
				else
					return false;
			}
		}
		return true;
		
	case kDataType_Float4:
		{
			if (json.IsObject() == false)
				return false;
			
			auto & value = plain_type->access<Vec4>(object);
			
			auto object = json.GetObject();
			
			for (auto member = object.begin(); member != object.end(); ++member)
			{
				if (member->name == "x")
					value[0] = member->value.GetFloat();
				else if (member->name == "y")
					value[1] = member->value.GetFloat();
				else if (member->name == "z")
					value[2] = member->value.GetFloat();
				else if (member->name == "w")
					value[3] = member->value.GetFloat();
				else
					return false;
			}
		}
		return true;
		
	case kDataType_Double:
		if (json.IsDouble() == false)
			return false;
		plain_type->access<double>(object) = json.GetDouble();
		return true;
		
	case kDataType_String:
		if (json.IsString() == false)
			return false;
		plain_type->access<std::string>(object) = json.GetString();
		return true;
		
	case kDataType_Enum:
		{
			if (json.IsString() == false)
				return false;
			
			auto * enum_type = static_cast<const EnumType*>(plain_type);
			
			if (enum_type->set(object, json.GetString()) == false)
			{
				LOG_ERR("failed to deserialize plain type enum key: %s", json.GetString());
				return false;
			}
			
			return true;
		}
		
	case kDataType_Other:
		Assert(false);
		break;
	}
	
	return false;
}

bool plain_type_tojson(const PlainType * plain_type, const void * object, REFLECTIONIO_JSON_WRITER & writer)
{
	switch (plain_type->dataType)
	{
	case kDataType_Bool:
		writer.Bool(plain_type->access<bool>(object));
		return true;
		
	case kDataType_Int:
		writer.Int(plain_type->access<int>(object));
		return true;
		
	case kDataType_Float:
		writer.Double(plain_type->access<float>(object));
		return true;
		
	case kDataType_Float2:
		{
			auto & value = plain_type->access<Vec2>(object);

			writer.StartObject();
			{
				writer.Key("x");
				writer.Double(value[0]);
				writer.Key("y");
				writer.Double(value[1]);
			}
			writer.EndObject();
		}
		return true;
		
	case kDataType_Float3:
		{
			auto & value = plain_type->access<Vec3>(object);

			writer.StartObject();
			{
				writer.Key("x");
				writer.Double(value[0]);
				writer.Key("y");
				writer.Double(value[1]);
				writer.Key("z");
				writer.Double(value[2]);
			}
			writer.EndObject();
		}
		return true;
		
	case kDataType_Float4:
		{
			auto & value = plain_type->access<Vec4>(object);

			writer.StartObject();
			{
				writer.Key("x");
				writer.Double(value[0]);
				writer.Key("y");
				writer.Double(value[1]);
				writer.Key("z");
				writer.Double(value[2]);
				writer.Key("w");
				writer.Double(value[3]);
			}
			writer.EndObject();
		}
		return true;
		
	case kDataType_Double:
		writer.Double(plain_type->access<double>(object));
		return true;
		
	case kDataType_String:
		writer.String(plain_type->access<std::string>(object).c_str());
		return true;
		
	case kDataType_Enum:
		{
			auto * enum_type = static_cast<const EnumType*>(plain_type);
			
			const char * key;
			if (enum_type->get_key(object, key) == false)
			{
				int value;
				if (!enum_type->get_value(object, value))
					value = -1;
				LOG_ERR("failed to find key for enum %s, value %d", enum_type->typeName, value);
				return false;
			}
			
			writer.String(key);
			
			return true;
		}
		
	case kDataType_Other:
		Assert(false);
		break;
	}
	
	return false;
}

//

bool member_tojson_recursive(
	const TypeDB & typeDB,
	const StructuredType * structured_type,
	const void * object,
	const Member * member,
	REFLECTIONIO_JSON_WRITER & writer)
{
	bool result = true;
	
	if (member->isVector)
	{
		auto * member_interface = static_cast<const Member_VectorInterface*>(member);
		
		auto * vector_type = typeDB.findType(member_interface->vector_type());
		
		if (vector_type == nullptr)
		{
			LOG_ERR("failed to find type for vector member %s", member_interface->name);
			result &= false;
		}
		else
		{
			writer.Key(member->name);
			
			const size_t vector_size = member_interface->vector_size(object);
			
			if (vector_type->isStructured)
			{
				writer.StartArray();
				{
					for (size_t i = 0; i < vector_size; ++i)
					{
						auto * vector_object = member_interface->vector_access((void*)object, i);
						
						result &= object_tojson_recursive(typeDB, vector_type, vector_object, writer);
					}
				}
				writer.EndArray();
			}
			else
			{
				const PlainType * plain_type = static_cast<const PlainType*>(vector_type);
				
				writer.StartArray();
				{
					for (size_t i = 0; i < vector_size; ++i)
					{
						auto * vector_object = member_interface->vector_access((void*)object, i);
						
						result &= plain_type_tojson(plain_type, vector_object, writer);
					}
				}
				writer.EndArray();
			}
		}
	}
	else
	{
		auto * member_scalar = static_cast<const Member_Scalar*>(member);
		
		auto * member_type = typeDB.findType(member_scalar->typeIndex);
		auto * member_object = member_scalar->scalar_access(object);
		
		if (member_type == nullptr)
		{
			LOG_ERR("failed to find type for member %s", member->name);
			result &= false;
		}
		else if (member->hasFlag<MemberFlag_CustomJsonSerialization>())
		{
			writer.Key(member->name);
			
			auto * customJsonSerialization = member->findFlag<MemberFlag_CustomJsonSerialization>();
			
			customJsonSerialization->tojson(typeDB, member, member_object, writer);
		}
		else
		{
			writer.Key(member->name);
			
			result &= object_tojson_recursive(typeDB, member_type, member_object, writer);
		}
	}
	
	return result;
}

bool object_fromjson_recursive(const TypeDB & typeDB, const Type * type, void * object, const rapidjson::Document::ValueType & json)
{
	if (type->isStructured)
	{
		bool result = true;
		
		auto * structured_type = static_cast<const StructuredType*>(type);
		
		auto object_json = json.GetObject();
		
		for (auto member_json = object_json.begin(); member_json != object_json.end(); ++member_json)
		{
			// determine the structured member name
			
			const char * name = member_json->name.GetString();
			
			// find the member inside the structured type
			
			Member * member = nullptr;
			
			for (auto * member_itr = structured_type->members_head; member_itr != nullptr; member_itr = member_itr->next)
			{
				if (strcmp(member_itr->name, name) == 0)
					member = member_itr;
			}
			
			if (member == nullptr)
			{
				// the member json contains data for a member we don't know. skip it
				
				LOG_WRN("unknown member: %s", name);
			}
			else
			{
				// deserialize the member
				
				result &= member_fromjson_recursive(typeDB, member, object, member_json->value);
			}
		}
		
		return result;
	}
	else
	{
		auto * plain_type = static_cast<const PlainType*>(type);
		
		if (plain_type_fromjson(plain_type, object, json) == false)
		{
			LOG_ERR("failed to deserialize plain type from json");
			
			return false;
		}
		else
		{
			return true;
		}
	}
	
	return false;
}

bool member_fromjson_recursive(const TypeDB & typeDB, const Member * member, void * object, const rapidjson::Document::ValueType & document)
{
	bool result = true;
	
	if (member->isVector)
	{
		auto * member_interface = static_cast<const Member_VectorInterface*>(member);
		
		member_interface->vector_resize(object, 0);
		
		auto * vector_type = typeDB.findType(member_interface->vector_type());

		if (vector_type->isStructured)
		{
			auto json_array = document.GetArray();
			
			member_interface->vector_resize(object, json_array.Size());
			
			int index = 0;
			
			for (auto json_element = json_array.begin(); json_element != json_array.end(); ++json_element, ++index)
			{
				auto * vector_object = member_interface->vector_access(object, index);
				
				result &= object_fromjson_recursive(typeDB, vector_type, vector_object, *json_element);
			}
		}
		else
		{
			// optimization for plain type data. call plain_type_fromjson(..) directly
			
			auto * plain_type = static_cast<const PlainType*>(vector_type);
			
			auto json_array = document.GetArray();
			
			member_interface->vector_resize(object, json_array.Size());
			
			int index = 0;
			
			for (auto json_element = json_array.begin(); json_element != json_array.end(); ++json_element, ++index)
			{
				auto * vector_object = member_interface->vector_access(object, index);
			
				result &= plain_type_fromjson(plain_type, vector_object, *json_element);
			}
		}
	}
	else
	{
		auto * member_scalar = static_cast<const Member_Scalar*>(member);

		auto * member_type = typeDB.findType(member_scalar->typeIndex);
		auto * member_object = member_scalar->scalar_access(object);
		
		if (member_type == nullptr)
		{
			LOG_ERR("failed to find type for member %s", member->name);
			result &= false;
		}
		else
		{
			if (object_fromjson_recursive(typeDB, member_type, member_object, document) == false)
			{
				LOG_ERR("failed to deserialize member %s", member->name);
				result &= false;
			}
		}
	}
	
	return result;
}

bool object_tojson_recursive(const TypeDB & typeDB, const Type * type, const void * object, REFLECTIONIO_JSON_WRITER & writer)
{
	if (type->isStructured)
	{
		bool result = true;
		
		auto * structured_type = static_cast<const StructuredType*>(type);
		
		writer.StartObject();
		{
			for (auto * member = structured_type->members_head; member != nullptr; member = member->next)
			{
				result &= member_tojson_recursive(typeDB, structured_type, object, member, writer);
			}
			
			if (result == false)
			{
				return false;
			}
		}
		writer.EndObject();
		
		return result;
	}
	else
	{
		auto * plain_type = static_cast<const PlainType*>(type);
		
		if (plain_type_tojson(plain_type, object, writer) == false)
		{
			LOG_ERR("failed to serialize plain type to text. typeName: %s", plain_type->typeName);
			return false;
		}
		else
		{
			return true;
		}
	}
	
	return false;
}

bool member_tojson_recursive(const TypeDB & typeDB, const Member * member, const void * object, REFLECTIONIO_JSON_WRITER & json, const int currentIndent)
{
	bool result = true;
	
	if (member->isVector)
	{
		LOG_ERR("vector types aren't supported yet");
		Assert(false);
		//result &= false;
	}
	else
	{
		auto * member_scalar = static_cast<const Member_Scalar*>(member);

		auto * member_type = typeDB.findType(member_scalar->typeIndex);
		auto * member_object = member_scalar->scalar_access(object);
		
		if (member_type == nullptr)
		{
			LOG_ERR("failed to find type for member %s", member->name);
			result &= false;
		}
		else if (member->hasFlag<MemberFlag_CustomJsonSerialization>())
		{
			AssertMsg(false, "not yet implemented");
			
			//auto * customJsonSerialization = member->findFlag<MemberFlag_CustomJsonSerialization>();
			
			//customJsonSerialization->tojson(typeDB, member, member_object, json);
		}
		else
		{
			result &= object_tojson_recursive(typeDB, member_type, member_object, json);
		}
	}
	
	return result;
}

MemberFlag_CustomJsonSerialization * customJsonSerializationFlag(MemberToJsonFunction tojson)
{
	auto * result = new MemberFlag_CustomJsonSerialization();
	
	result->tojson = tojson;
	
	return result;
}

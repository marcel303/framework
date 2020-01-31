#include "reflection.h"
#include "reflection-textio.h"

#include "lineReader.h"
#include "lineWriter.h"

#include "Debugging.h"
#include "Log.h"
#include "Parse.h" // todo : remove this dependency
#include "StringEx.h" // strcpy_s
#include "Vec2.h" // plain type supported for serialization
#include "Vec3.h" // plain type supported for serialization
#include "Vec4.h" // plain type supported for serialization

inline bool is_whitespace(const char c)
{
	return c == ' ' || c == '\t';
}

static bool extract_word(const char *& in_text, char * __restrict out_word, const int max_word_size)
{
	const char * __restrict text = in_text;
	
	while (*text != 0 && is_whitespace(*text) == true)
		text++;
	
	if (*text == 0)
		return false;
	
	const char * word = text;
	
	while (*text != 0 && is_whitespace(*text) == false)
		text++;
	
	if (text > word)
	{
		const size_t size = text - word;
		
		if (*text != 0)
		{
			text++;
		}
		
		if (size + 1 > max_word_size)
			return false;
		else
		{
			for (size_t i = 0; i < size; ++i)
				out_word[i] = word[i];
			
			out_word[size] = 0;
			
			in_text = text;
			
			return true;
		}
	}
	else
	{
		return false;
	}
}

bool plain_type_fromtext(const PlainType * plain_type, void * object, const char * text)
{
// todo : I definitely need better parse functions. ones which return a success code

	switch (plain_type->dataType)
	{
	case kDataType_Bool:
		plain_type->access<bool>(object) = Parse::Bool(text);
		return true;
		
	case kDataType_Int:
		plain_type->access<int>(object) = Parse::Int32(text);
		return true;
		
	case kDataType_Float:
		plain_type->access<float>(object) = Parse::Float(text);
		return true;
		
	case kDataType_Float2:
		{
			char parts[2][64];
			
			const char * text_ptr = text;
			
			if (!extract_word(text_ptr, parts[0], sizeof(parts[0])) ||
				!extract_word(text_ptr, parts[1], sizeof(parts[1])))
			{
				return false;
			}
			
			plain_type->access<Vec2>(object).Set(
				Parse::Float(parts[0]),
				Parse::Float(parts[1]));
		}
		return true;
		
	case kDataType_Float3:
		{
			char parts[3][64];
			
			const char * text_ptr = text;
			
			if (!extract_word(text_ptr, parts[0], sizeof(parts[0])) ||
				!extract_word(text_ptr, parts[1], sizeof(parts[1])) ||
				!extract_word(text_ptr, parts[2], sizeof(parts[2])))
			{
				return false;
			}
			
			plain_type->access<Vec3>(object).Set(
				Parse::Float(parts[0]),
				Parse::Float(parts[1]),
				Parse::Float(parts[2]));
		}
		return true;
		
	case kDataType_Float4:
		{
			char parts[4][64];
			
			const char * text_ptr = text;
			
			if (!extract_word(text_ptr, parts[0], sizeof(parts[0])) ||
				!extract_word(text_ptr, parts[1], sizeof(parts[1])) ||
				!extract_word(text_ptr, parts[2], sizeof(parts[2])) ||
				!extract_word(text_ptr, parts[3], sizeof(parts[3])))
			{
				return false;
			}
			
			plain_type->access<Vec4>(object).Set(
				Parse::Float(parts[0]),
				Parse::Float(parts[1]),
				Parse::Float(parts[2]),
				Parse::Float(parts[3]));
		}
		return true;
		
	case kDataType_Double:
		plain_type->access<double>(object) = Parse::Float(text); // todo : parse doubles using more accuracy
		return true;
		
	case kDataType_String:
		plain_type->access<std::string>(object) = text;
		return true;
		
	case kDataType_Enum:
		{
			auto * enum_type = static_cast<const EnumType*>(plain_type);
			
			if (enum_type->set(object, text) == false)
				return false;
			
			return true;
		}
		
	case kDataType_Other:
		Assert(false);
		break;
	}
	
	return false;
}

#include "rapidjson/internal/dtoa.h"
#include "rapidjson/internal/itoa.h"

bool plain_type_totext(const PlainType * plain_type, const void * object, char * out_text, const int out_text_size)
{
	switch (plain_type->dataType)
	{
	case kDataType_Bool:
		strcpy_s(out_text, out_text_size, plain_type->access<bool>(object) ? "true" : "false");
		return true;
		
	case kDataType_Int:
		{
			char * text_ptr = rapidjson::internal::i32toa(plain_type->access<int>(object), out_text);
			*text_ptr = 0;
		}
		return true;
		
	case kDataType_Float:
		{
			char * text_ptr = rapidjson::internal::dtoa(plain_type->access<float>(object), out_text);
			*text_ptr++ = 0;
		}
		return true;
		
	case kDataType_Float2:
		{
			auto & value = plain_type->access<Vec2>(object);
			
			char * text_ptr = out_text;
			text_ptr = rapidjson::internal::dtoa(value[0], text_ptr); *text_ptr++ = ' ';
			text_ptr = rapidjson::internal::dtoa(value[1], text_ptr); *text_ptr++ = 0;
		}
		return true;
		
	case kDataType_Float3:
		{
			auto & value = plain_type->access<Vec3>(object);
			
			char * text_ptr = out_text;
			text_ptr = rapidjson::internal::dtoa(value[0], text_ptr); *text_ptr++ = ' ';
			text_ptr = rapidjson::internal::dtoa(value[1], text_ptr); *text_ptr++ = ' ';
			text_ptr = rapidjson::internal::dtoa(value[2], text_ptr); *text_ptr++ = 0;
		}
		return true;
		
	case kDataType_Float4:
		{
			auto & value = plain_type->access<Vec4>(object);
			
			char * text_ptr = out_text;
			text_ptr = rapidjson::internal::dtoa(value[0], text_ptr); *text_ptr++ = ' ';
			text_ptr = rapidjson::internal::dtoa(value[1], text_ptr); *text_ptr++ = ' ';
			text_ptr = rapidjson::internal::dtoa(value[2], text_ptr); *text_ptr++ = ' ';
			text_ptr = rapidjson::internal::dtoa(value[3], text_ptr); *text_ptr++ = 0;
		}
		return true;
		
	case kDataType_Double:
		{
			char * text_ptr = rapidjson::internal::dtoa(plain_type->access<double>(object), out_text);
			*text_ptr++ = 0;
		}
		return true;
		
	case kDataType_String:
		strcpy_s(out_text, out_text_size, plain_type->access<std::string>(object).c_str());
		return true;
		
	case kDataType_Enum:
		{
			auto * enum_type = static_cast<const EnumType*>(plain_type);
			
			const char * key;
			if (enum_type->get_key(object, key) == false)
				return false;
			
			strcpy_s(out_text, out_text_size, key);
			
			return true;
		}
		
	case kDataType_Other:
		Assert(false);
		break;
	}
	
	return false;
}

//

bool member_tolines_recursive(
	const TypeDB & typeDB,
	const StructuredType * structured_type,
	const void * object,
	const Member * member,
	LineWriter & line_writer,
	const int currentIndent)
{
	bool result = true;
	
	if (member->isVector)
	{
		auto * member_interface = static_cast<const Member_VectorInterface*>(member);
		
		auto * vector_type = typeDB.findType(member_interface->vector_type());
		
		if (vector_type == nullptr)
		{
			LOG_ERR("failed to find type for type name %s", structured_type->typeName);
			result &= false;
		}
		else
		{
			const size_t vector_size = member_interface->vector_size(object);
			
			if (vector_type->isStructured)
			{
				for (size_t i = 0; i < vector_size; ++i)
				{
					auto * vector_object = member_interface->vector_access((void*)object, i);
					
					line_writer.append_indented_line(currentIndent, "-");
					
					result &= object_tolines_recursive(typeDB, vector_type, vector_object, line_writer, currentIndent + 1);
				}
			}
			else
			{
				for (size_t i = 0; i < vector_size; ++i)
				{
					auto * vector_object = member_interface->vector_access((void*)object, i);
					
					result &= object_tolines_recursive(typeDB, vector_type, vector_object, line_writer, currentIndent);
				}
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
			result &= object_tolines_recursive(typeDB, member_type, member_object, line_writer, currentIndent);
		}
	}
	
	return result;
}

bool object_fromlines_recursive(
	const TypeDB & typeDB, const Type * type, void * object,
	LineReader & line_reader)
{
	if (type->isStructured)
	{
		bool result = true;
		
		auto * structured_type = static_cast<const StructuredType*>(type);
		
		const char * line;
		
		while ((line = line_reader.get_next_line(true)) != nullptr)
		{
			if (line[0] == '\t')
			{
				// only one level of identation may be added per line
				
				LOG_ERR("more than one level of identation added on line %d", line_reader.get_current_line_index());
				return false;
			}
			
			// determine the structured member name
			
			const char * name = line;
			
			// find the member inside the structured type
			
			Member * member = nullptr;
			
			for (auto * member_itr = structured_type->members_head; member_itr != nullptr; member_itr = member_itr->next)
			{
				if (strcmp(member_itr->name, name) == 0)
					member = member_itr;
			}
			
			if (member == nullptr)
			{
				// the lines contain data for a member we don't know. skip it
				
				LOG_WRN("unknown member: %s", name);
				
				line_reader.push_indent();
				{
					while (line_reader.get_next_line(true))
					{
						// skip indented lines
					}
				}
				line_reader.pop_indent();
			}
			else
			{
				// deserialize the member
				
				line_reader.push_indent();
				{
					result &= member_fromlines_recursive(typeDB, member, object, line_reader);
				}
				line_reader.pop_indent();
			}
		}
		
		return result;
	}
	else
	{
		const char * line = line_reader.get_next_line(false);
		
		AssertMsg(line != nullptr, "got empty line for plain type", 0);
		
		if (line == nullptr)
		{
			return false;
		}
		else if (line[0] == '\t')
		{
			// only one level of identation may be added per line
			
			LOG_ERR("more than one level of identation added on line %d", line_reader.get_current_line_index());
			return false;
		}
		else
		{
			auto * plain_type = static_cast<const PlainType*>(type);
			
			if (plain_type_fromtext(plain_type, object, line) == false)
			{
				LOG_ERR("failed to deserialize plain type from text", 0);
				
				return false;
			}
			else
			{
				return true;
			}
		}
	}
	
	return false;
}

bool member_fromlines_recursive(
	const TypeDB & typeDB, const Member * member, void * object,
	LineReader & line_reader)
{
	bool result = true;
	
	if (member->isVector)
	{
		auto * member_interface = static_cast<const Member_VectorInterface*>(member);
		
		member_interface->vector_resize(object, 0);
		
		auto * vector_type = typeDB.findType(member_interface->vector_type());

		if (vector_type->isStructured)
		{
			const char * element;
			
			while ((element = line_reader.get_next_line(true)))
			{
				Assert(element[0] == '-');
				if (element[0] != '-')
				{
					LOG_ERR("syntax error. expected '-' for next array element", 0);
					result &= false;
				}
				else if (element[0] == '\t')
				{
					// only one level of identation may be added per line
					
					LOG_ERR("more than one level of identation added on line %d", line_reader.get_current_line_index());
					return false;
				}
				else
				{
					member_interface->vector_resize(object, member_interface->vector_size(object) + 1);
					
					auto * vector_object = member_interface->vector_access(object, member_interface->vector_size(object) - 1);
					
					line_reader.push_indent();
					{
						result &= object_fromlines_recursive(typeDB, vector_type, vector_object, line_reader);
					}
					line_reader.pop_indent();
				}
			}
		}
		else
		{
			// more condensed format for plain data
			
			auto * plain_type = static_cast<const PlainType*>(vector_type);
			
			const char * element;
			
			while ((element = line_reader.get_next_line(true)))
			{
				if (element[0] == '\t')
				{
					// only one level of identation may be added per line
					
					LOG_ERR("more than one level of identation added on line %d", line_reader.get_current_line_index());
					return false;
				}
				
				member_interface->vector_resize(object, member_interface->vector_size(object) + 1);
				
				auto * vector_object = member_interface->vector_access(object, member_interface->vector_size(object) - 1);
			
				result &= plain_type_fromtext(plain_type, vector_object, element);
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
			result &= object_fromlines_recursive(typeDB, member_type, member_object, line_reader);
		}
	}
	
	return result;
}

bool object_tolines_recursive(
	const TypeDB & typeDB, const Type * type, const void * object,
	LineWriter & line_writer, const int currentIndent)
{
	if (type->isStructured)
	{
		bool result = true;
		
		auto * structured_type = static_cast<const StructuredType*>(type);
		
		for (auto * member = structured_type->members_head; member != nullptr; member = member->next)
		{
			line_writer.append_indented_line(currentIndent, member->name);
			
			result &= member_tolines_recursive(typeDB, structured_type, object, member, line_writer, currentIndent + 1);
		}
		
		return result;
	}
	else
	{
		auto * plain_type = static_cast<const PlainType*>(type);
		
		const int text_size = 1024;
		char text[text_size];
		
		if (plain_type_totext(plain_type, object, text, text_size) == false)
		{
			LOG_ERR("failed to serialize plain type to text", 0);
			return false;
		}
		else
		{
			line_writer.append_indented_line(currentIndent, text);
			return true;
		}
	}
	
	return false;
}

bool member_tolines_recursive(
	const TypeDB & typeDB, const Member * member, const void * object,
	LineWriter & line_writer, const int currentIndent)
{
	bool result = true;
	
	if (member->isVector)
	{
	// todo : add vector support
		LOG_ERR("vector types aren't supported yet", 0);
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
		else
		{
			result &= object_tolines_recursive(typeDB, member_type, member_object, line_writer, currentIndent);
		}
	}
	
	return result;
}

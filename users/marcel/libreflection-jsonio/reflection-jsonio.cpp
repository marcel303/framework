#include "Debugging.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/writer.h"
#include "reflection.h"
#include "reflection-jsonio.h"
#include "Vec2.h"
#include "Vec3.h"
#include "Vec4.h"
#include <string>

using namespace rapidjson;

bool plain_type_fromjson(const PlainType * plain_type, void * object, const char * json)
{
	return false;
}

bool plain_type_tojson(const PlainType * plain_type, const void * object, rapidjson::StringBuffer & out_json)
{
	Writer<StringBuffer> writer(out_json);

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
		
	case kDataType_Other:
		Assert(false);
		break;
	}
	
	return false;
}

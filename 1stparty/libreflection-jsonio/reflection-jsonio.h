#pragma once

#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

//#define REFLECTIONIO_JSON_WRITER rapidjson::Writer<rapidjson::StringBuffer>
#define REFLECTIONIO_JSON_WRITER rapidjson::PrettyWriter<rapidjson::StringBuffer>

// forward declarations

struct Member;
struct MemberFlag_CustomJsonSerialization;
struct PlainType;
struct StructuredType;
struct Type;
struct TypeDB;

// plain type from and to json

bool plain_type_fromjson(const PlainType * plain_type, void * object, const rapidjson::Document::ValueType & document);

bool plain_type_tojson(const PlainType * plain_type, const void * object, REFLECTIONIO_JSON_WRITER & out_json);

// structured type from and to json

bool member_tojson_recursive(const TypeDB & typeDB, const StructuredType * structured_type, const void * object, const Member * member, rapidjson::StringBuffer & json);

bool object_fromjson_recursive(const TypeDB & typeDB, const Type * type, void * object, const rapidjson::Document::ValueType & document);
bool member_fromjson_recursive(const TypeDB & typeDB, const Member * member, void * object, const rapidjson::Document::ValueType & document);

bool object_tojson_recursive(const TypeDB & typeDB, const Type * type, const void * object, REFLECTIONIO_JSON_WRITER & json);
bool member_tojson_recursive(const TypeDB & typeDB, const Member * member, const void * object, REFLECTIONIO_JSON_WRITER & json);

// type DB

#include "reflection.h"

typedef bool (*MemberToJsonFunction)(const TypeDB & typeDB, const Member * member, const void * member_object, REFLECTIONIO_JSON_WRITER & writer);

struct MemberFlag_CustomJsonSerialization : MemberFlag<MemberFlag_CustomJsonSerialization>
{
	MemberToJsonFunction tojson = nullptr;
};

MemberFlag_CustomJsonSerialization * customJsonSerializationFlag(MemberToJsonFunction tojson);

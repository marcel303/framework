#pragma once

#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"

// forward declarations

struct Member;
struct PlainType;
struct StructuredType;
struct Type;
struct TypeDB;

// plain type from and to json

bool plain_type_fromjson(const PlainType * plain_type, void * object, const rapidjson::Document & document);

bool plain_type_tojson(const PlainType * plain_type, const void * object, rapidjson::StringBuffer & out_json);

#if 0 // todo : implement structured types to and from json

// structured type from and to json

bool member_tojson_recursive(const TypeDB & typeDB, const StructuredType * structured_type, const void * object, const Member * member, LineWriter & line_writer, const int currentIndent);

bool object_fromjson_recursive(
	const TypeDB & typeDB, const Type * type, void * object,
	LineReader & line_reader);
bool member_fromjson_recursive(
	const TypeDB & typeDB, const Member * member, void * object,
	LineReader & line_reader);

bool object_tojson_recursive(
	const TypeDB & typeDB, const Type * type, const void * object,
	LineWriter & line_Writer, const int currentIndent);
bool member_tojson_recursive(
	const TypeDB & typeDB, const Member * member, const void * object,
	LineWriter & line_Writer, const int currentIndent);

#endif

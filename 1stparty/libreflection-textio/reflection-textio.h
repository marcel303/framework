#pragma once

// forward declarations

class LineReader;
class LineWriter;

struct Member;
struct PlainType;
struct StructuredType;
struct Type;
struct TypeDB;

// plain type from and to text

bool plain_type_fromtext(
	const PlainType * plain_type,
	void * object,
	const char * text);

bool plain_type_totext(
	const PlainType * plain_type,
	const void * object,
	char * out_text,
	const int out_text_size);

// structured type from and to text

bool object_fromlines_recursive(
	const TypeDB & typeDB,
	const Type * type,
	void * object,
	LineReader & line_reader);
bool member_fromlines_recursive(
	const TypeDB & typeDB,
	const Member * member,
	void * object,
	LineReader & line_reader);

bool object_tolines_recursive(
	const TypeDB & typeDB,
	const Type * type,
	const void * object,
	LineWriter & line_Writer,
	const int currentIndent);
bool member_tolines_recursive(
	const TypeDB & typeDB,
	const Member * member,
	const void * object,
	LineWriter & line_Writer,
	const int currentIndent);

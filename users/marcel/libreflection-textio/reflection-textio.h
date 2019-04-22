#pragma once

class LineReader;
class LineWriter;

struct Member;
struct PlainType;
struct StructuredType;
struct Type;
struct TypeDB;

bool object_fromtext(const TypeDB & typeDB, const PlainType * plain_type, void * object, const char * text);

bool member_tolines_recursive(const TypeDB & typeDB, const StructuredType * structured_type, const void * object, const Member * member, LineWriter & line_writer, const int currentIndent);

bool object_fromlines_recursive(
	const TypeDB & typeDB, const Type * type, void * object,
	LineReader & line_reader);
bool member_fromlines_recursive(
	const TypeDB & typeDB, const Member * member, void * object,
	LineReader & line_reader);

bool object_tolines_recursive(
	const TypeDB & typeDB, const Type * type, const void * object,
	LineWriter & line_Writer, const int currentIndent);
bool member_tolines_recursive(
	const TypeDB & typeDB, const Member * member, const void * object,
	LineWriter & line_Writer, const int currentIndent);

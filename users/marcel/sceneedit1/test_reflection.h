#pragma once

/*

structured type info contains information about a structured type
(yes, really!)

structured types must support:
	members: using plain old data types
	arrays: using any plain old data type or array type

design decision: we will want type-safety for arrays. so we will
need to support templated array types

we will need some way to fetch type information from c++

we will need some way to fetch type information from text
	WELL ACTUALLY
		scratch this. we only need type names for things we instance explicitly by
		type. which currently, is just the components
	WHAT WE DO NEED
		is the ability to reflect on a type, and have some way to iterate over its members
		find out if it's an array or not, and to serialize and deserialize data
		we will also need to known the names of types for editing, and for serialization

inferring the type from c++ would only be needed for editing. we will need to get
the std::type_index for this for relection info

StructuredType
	StructuredType (cyclic-constraint) [] members
	type_index
	name

(cyclic-constraint) means it must be possible to have cyclic references between types
	structure x with array of x should be possible
	this implies we need to have pointers for StructuredType info

*/

#include <typeindex>

struct Member
{
	Member * next;

	const char * name;
	bool isVector;

	Member(const char * in_name, const bool in_isVector)
		: next(nullptr)
		, name(in_name)
		, isVector(in_isVector)
	{
	}
};

struct Member_Scalar : Member
{
	std::type_index typeIndex;
	size_t offset;
	
	Member_Scalar(const char * in_name, const std::type_index & in_typeIndex, const size_t in_offset)
		: Member(in_name, false)
		, typeIndex(in_typeIndex)
		, offset(in_offset)
	{
		
	}
	void * scalar_access(void * object)
	{
		const uintptr_t address = (uintptr_t)object;
		
		return (void*)(address + offset);
	}
};

struct Member_VectorInterface : Member
{
	virtual std::type_index vector_type() const = 0;
	virtual size_t vector_size(const void * object) const = 0;
	virtual void vector_resize(void * object, const size_t size) = 0;
	virtual void * vector_access(void * object, const size_t index) const = 0;
};

struct Type
{
	bool isStructured;

	Type(const bool in_isStructured)
		: isStructured(in_isStructured)
	{
	}
};

enum DataType
{
	kDataType_Bool,
	kDataType_Int,
	kDataType_Float,
	kDataType_Vec2,
	kDataType_Vec3,
	kDataType_Vec4,
	kDataType_String // std::string
};

struct PlainType : Type
{
	const char * typeName;
	DataType dataType;

	PlainType(const char * in_typeName, const DataType in_dataType)
		: Type(false)
		, typeName(in_typeName)
		, dataType(in_dataType)
	{
	}
	
	template <typename T>
	T & access(void * object) const
	{
		// todo : perform data type validation ?
		
		return *(T*)object;
	}
};

struct StructuredType : Type
{
	const char * typeName;
	Member * members_head;
	
	StructuredType(const char * in_typeName)
		: Type(true)
		, typeName(in_typeName)
		, members_head(nullptr)
	{
	}
	
	void add(const std::type_index & typeIndex, const size_t offset, const char * name);
};

struct TypeDB
{
	struct TypeDB_impl * impl;

	TypeDB();
	~TypeDB();
	
	const Type * findType(const std::type_index & typeIndex) const;

	template <typename T>
	const Type * findType() const
	{
		return findType(std::type_index(typeid(T)));
	}
	
	template <typename T>
	const Type * findType(const T & ref) const
	{
		return findType(std::type_index(typeid(T)));
	}

	void add(const std::type_index & typeIndex, const Type * type);

	template <typename T>
	Type * addPlain(const char * typeName, const DataType dataType)
	{
		PlainType * type = new PlainType(typeName, dataType);

		add(std::type_index(typeid(T)), type);
		
		return type;
	}
};

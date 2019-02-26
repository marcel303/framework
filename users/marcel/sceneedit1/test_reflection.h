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

#include <string>
#include <typeindex>
#include <vector>

struct Member
{
	std::string name; // todo : make const char*
	bool isVector;
};

struct Member_ScalarInterface : Member
{
	virtual std::type_index scalar_type() = 0;
	virtual void * scalar_access(void * object) = 0;
};

struct Member_Scalar : Member_ScalarInterface
{
	std::type_index typeIndex;
	size_t offset = -1;
	
	Member_Scalar(const std::type_index in_typeIndex)
		: typeIndex(in_typeIndex)
	{
		
	}
	virtual std::type_index scalar_type()
	{
		return typeIndex;
	}
	
	virtual void * scalar_access(void * object)
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
	bool isStructured = false;
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
	std::string typeName;
	DataType dataType;
	
	template <typename T>
	T & access(void * object) const
	{
		// todo : perform data type validation ?
		
		return *(T*)object;
	}
};

struct StructuredType : Type
{
	std::string typeName;
	
	// todo : use linked list ?
	std::vector<Member*> members;
	
	StructuredType(const char * in_typeName)
	{
		isStructured = true;
		typeName = in_typeName;
	}
	
	void add(std::type_index typeIndex, const size_t offset, const char * name)
	{
		Member_Scalar * member = new Member_Scalar(typeIndex);
		member->name = name;
		member->isVector = false;
		member->offset = offset;
		
		members.push_back(member);
	}
};

#include <map>

struct TypeDB
{
	std::map<std::type_index, const Type*> types; // todo : use p-impl (ugh)
	
	const Type * findType(std::type_index typeIndex) const
	{
		auto i = types.find(typeIndex);

		if (i == types.end())
			return nullptr;
		else
			return i->second;
	}

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

	void add(std::type_index typeIndex, const Type * type)
	{
		types[typeIndex] = type;
	}

	template <typename T>
	Type * addPlain(const DataType dataType)
	{
		auto typeIndex = std::type_index(typeid(T));

		PlainType * type = new PlainType();
		type->isStructured = false;
		type->dataType = dataType;

		add(typeIndex, type);
		
		return type;
	}
};

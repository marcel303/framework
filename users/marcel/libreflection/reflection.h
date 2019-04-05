/*
	Copyright (C) 2019 Marcel Smit
	marcel303@gmail.com
	https://www.facebook.com/marcel.smit981

	Permission is hereby granted, free of charge, to any person
	obtaining a copy of this software and associated documentation
	files (the "Software"), to deal in the Software without
	restriction, including without limitation the rights to use,
	copy, modify, merge, publish, distribute, sublicense, and/or
	sell copies of the Software, and to permit persons to whom the
	Software is furnished to do so, subject to the following
	conditions:

	The above copyright notice and this permission notice shall be
	included in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
	OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
	NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
	HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
	WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
	OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <stddef.h>
#include <stdint.h>
#include <typeindex>

struct MemberFlagBase
{
	MemberFlagBase * next;
	
	std::type_index typeIndex;
	
	template <typename T>
	bool isType() const
	{
		return std::type_index(typeid(T)) == typeIndex;
	}
	
	MemberFlagBase(const std::type_index & in_typeIndex)
		: next(nullptr)
		, typeIndex(in_typeIndex)
	{
	}
};

template <typename T>
struct MemberFlag : MemberFlagBase
{
	MemberFlag()
		: MemberFlagBase(std::type_index(typeid(T)))
	{
	}
};

struct Member
{
	Member * next;

	const char * name;
	bool isVector;
	
	MemberFlagBase * flags;
	
	Member(const char * in_name, const bool in_isVector)
		: next(nullptr)
		, name(in_name)
		, isVector(in_isVector)
		, flags(nullptr)
	{
	}
	
	virtual ~Member()
	{
	}
	
	void addFlag(MemberFlagBase * flag)
	{
		flag->next = flags;
		flags = flag;
	}
	
	template <typename T>
	const T * findFlag() const
	{
		const std::type_index typeIndex(typeid(T));
		
		for (auto * flag = flags; flag != nullptr; flag = flag->next)
			if (flag->typeIndex == typeIndex)
				return static_cast<const T*>(flag);
		
		return nullptr;
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
	
	void * scalar_access(void * object) const
	{
		const uintptr_t address = (uintptr_t)object;
		
		return (void*)(address + offset);
	}
	
	const void * scalar_access(const void * object) const
	{
		const uintptr_t address = (uintptr_t)object;
		
		return (void*)(address + offset);
	}
};

#include "reflection-vector.h" // support for std::vector<T> is defined in a separate file to keep things a bit more tidy

struct Type
{
	bool isStructured;

	Type(const bool in_isStructured)
		: isStructured(in_isStructured)
	{
	}
	
	virtual ~Type()
	{
	}
};

enum DataType // just for convenience and efficiency reasons, this enum allows for fast identification of plain types. so instead of using string comparisons on the type name, this enum can be used to more quickly determine the type
{
	kDataType_Other,
	kDataType_Bool,
	kDataType_Int,
	kDataType_Float,
	kDataType_Float2,
	kDataType_Float3,
	kDataType_Float4,
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
	
	template <typename T>
	const T & access(const void * object) const
	{
		// todo : perform data type validation ?
		
		return *(T*)object;
	}
};

struct StructuredType : Type
{
	const char * typeName;
	Member * members_head;
	Member * members_tail;
	
	StructuredType(const char * in_typeName)
		: Type(true)
		, typeName(in_typeName)
		, members_head(nullptr)
		, members_tail(nullptr)
	{
	}
	
	virtual ~StructuredType() override;
	
	void add(Member * member);
	
	StructuredType & add(const std::type_index & typeIndex, const size_t offset, const char * name);
	
	template <typename C, typename T>
	StructuredType & add(const char * name, std::vector<T> C::* C_member)
	{
		C * x = nullptr;
		const size_t offset = (size_t)(uintptr_t)&(x->*C_member);
		
		Member * member = new Member_Vector<T>(name, offset);
		
		add(member);
		
		return *this;
	}
	
	template <typename C, typename T>
	StructuredType & add(const char * name, T C::* C_member)
	{
		C * x = nullptr;
		const size_t offset = (size_t)(uintptr_t)&(x->*C_member);
		
		return add(std::type_index(typeid(T)), offset, name);
	}
	
	StructuredType & addFlag(MemberFlagBase * flag)
	{
		members_tail->addFlag(flag);
		
		return *this;
	}
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

	void add(const std::type_index & typeIndex, Type * type);
	StructuredType & add(const std::type_index & typeIndex, const char * typeName);
	
	template <typename T>
	PlainType & addPlain(const char * typeName, const DataType dataType)
	{
		PlainType * type = new PlainType(typeName, dataType);

		add(std::type_index(typeid(T)), type);
		
		return *type;
	}
	
	template <typename T>
	StructuredType & addStructured(const char * typeName)
	{
		StructuredType * type = new StructuredType(typeName);
		
		add(std::type_index(typeid(T)), type);
		
		return *type;
	}
};

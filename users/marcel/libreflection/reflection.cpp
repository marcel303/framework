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

#include "reflection.h"

//

StructuredType::~StructuredType()
{
	for (auto * member = members_head; member != nullptr; )
	{
		auto * next = member->next;
		
		for (auto * flag = member->flags; flag != nullptr; )
		{
			auto * next_flag = flag->next;
			
			delete flag;
			
			flag = next_flag;
		}
		
		delete member;
		
		member = next;
	}
}

void StructuredType::add(Member * member)
{
	if (members_head == nullptr)
	{
		members_head = member;
		members_tail = member;
	}
	else
	{
		members_tail->next = member;
		members_tail = member;
	}
}

StructuredType & StructuredType::add(const std::type_index & typeIndex, const size_t offset, const char * name)
{
	Member_Scalar * member = new Member_Scalar(name, typeIndex, offset);

	add(member);
	
	return *this;
}

//

#include <map>

struct TypeDB_impl
{
	std::map<std::type_index, const Type*> types;
	
	~TypeDB_impl()
	{
		for (auto & i : types)
			delete i.second;
	}
	
	const Type * findType(const std::type_index & typeIndex) const
	{
		auto i = types.find(typeIndex);

		if (i == types.end())
			return nullptr;
		else
			return i->second;
	}

	void add(std::type_index typeIndex, const Type * type)
	{
	// todo : assert type does not exist yet
	
		types[typeIndex] = type;
	}
};

TypeDB::TypeDB()
{
	impl = new TypeDB_impl();
}

TypeDB::~TypeDB()
{
	delete impl;
	impl = nullptr;
}

const Type * TypeDB::findType(const std::type_index & typeIndex) const
{
	return impl->findType(typeIndex);
}

void TypeDB::add(const std::type_index & typeIndex, Type * type)
{
	impl->add(typeIndex, type);
}

StructuredType & TypeDB::add(const std::type_index & typeIndex, const char * typeName)
{
	StructuredType * type = new StructuredType(typeName);
	
	add(typeIndex, type);
	
	return *type;
}

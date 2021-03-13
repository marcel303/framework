/*
	Copyright (C) 2020 Marcel Smit
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
#include <assert.h>
#include <string.h>

//

EnumType & EnumType::add(const char * key, const int value)
{
	EnumElem * elem = new EnumElem();
	elem->key = key;
	elem->value = value;
	elem->next = nullptr;
	
	EnumElem ** tail = &firstElem;
	while (*tail)
		tail = &(*tail)->next;
	*tail = elem;
	
	return *this;
}

EnumType::~EnumType()
{
	EnumElem * elem = firstElem;
	
	while (elem != nullptr)
	{
		EnumElem * next_elem = elem->next;
		
		delete elem;
		
		elem = next_elem;
	}
}

bool EnumType::set(void * object, const char * key) const
{
	for (const EnumElem * elem = firstElem; elem != nullptr; elem = elem->next)
	{
		if (strcmp(elem->key, key) == 0)
		{
			if (enumSize == 1)
				*(int8_t*)object = elem->value;
			else if (enumSize == 2)
				*(int16_t*)object = elem->value;
			else if (enumSize == 4)
				*(int32_t*)object = elem->value;
			else
				assert(false);
			
			return true;
		}
	}
	
	return false;
}

bool EnumType::get_key(const void * object, const char *& key) const
{
	int value;
	
	if (get_value(object, value) == false)
		return false;
	
	for (const EnumElem * elem = firstElem; elem != nullptr; elem = elem->next)
	{
		if (elem->value == value)
		{
			key = elem->key;
			return true;
		}
	}
	
	return false;
}

bool EnumType::get_value(const void * object, int & value) const
{
	if (enumSize == 1)
		value = *(int8_t*)object;
	else if (enumSize == 2)
		value = *(int16_t*)object;
	else if (enumSize == 4)
		value = *(int32_t*)object;
	else
	{
		assert(false);
		return false;
	}
	
	return true;
}

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

	void add(const std::type_index & typeIndex, const Type * type)
	{
		assert(types.find(typeIndex) == types.end());
	
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

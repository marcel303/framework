#include "reflection.h"

//

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

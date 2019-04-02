#pragma once

#include "reflection.h"
#include <vector>

struct Member_VectorInterface : Member
{
	Member_VectorInterface(const char * in_name, const bool in_isVector)
		: Member(in_name, in_isVector)
	{
	}
	
	virtual std::type_index vector_type() const = 0;
	virtual size_t vector_size(const void * object) const = 0;
	virtual void vector_resize(void * object, const size_t size) const = 0;
	virtual void * vector_access(void * object, const size_t index) const = 0;
	virtual void vector_swap(void * object, const size_t index1, const size_t index2) const = 0;
};

template <typename T>
struct Member_Vector : Member_VectorInterface
{
	typedef std::vector<T> VectorType;
	
	std::type_index typeIndex;
	size_t offset;
	
	Member_Vector(const char * in_name, const size_t in_offset)
		: Member_VectorInterface(in_name, true)
		, typeIndex(typeid(VectorType))
		, offset(in_offset)
	{
	}
	
	VectorType & getVector(void * object) const
	{
		const uintptr_t address = (uintptr_t)object;
		
		object = (void*)(address + offset);
		
		return *(VectorType*)object;
	}
	
	const VectorType & getVector(const void * object) const
	{
		const uintptr_t address = (uintptr_t)object;
		
		object = (void*)(address + offset);
		
		return *(VectorType*)object;
	}
	
	virtual std::type_index vector_type() const override final
	{
		return std::type_index(typeid(T));
	}
	
	virtual size_t vector_size(const void * object) const override final
	{
		return getVector(object).size();
	}
	
	virtual void vector_resize(void * object, const size_t size) const override final
	{
		return getVector(object).resize(size);
	}
	
	virtual void * vector_access(void * object, const size_t index) const override final
	{
		return &getVector(object).at(index);
	}
	
	virtual void vector_swap(void * object, const size_t index1, const size_t index2) const override final
	{
		auto & vector = getVector(object);
		
		std::swap(vector[index1], vector[index2]);
	}
};

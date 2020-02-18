#include "HashSpace.h"

ObjectPool<HashEntryItem> g_hashEntryItemPool;

template<typename T>
T * ObjectPool<T>::alloc()
{
	Element * element;
	
	if (first)
	{
		element = first;
		first = first->next;
	}
	else
		element = (Element*)malloc(sizeof(T));
	
	T * object = (T*)element;
	new (object) T();
	
	return object;
}

template<typename T>
void ObjectPool<T>::free(T * object)
{
	object->~T();
	
	Element * element = (Element*)object;
	element->next = first;
	first = element;
}

template class ObjectPool<HashEntryItem>;

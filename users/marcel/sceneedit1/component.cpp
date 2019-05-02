#include "component.h"
#include <string.h>

ComponentBase::~ComponentBase()
{
#if ENABLE_COMPONENT_IDS
	if (id[0] != 0)
	{
		delete [] id;
	}
#endif
}

void ComponentBase::setId(const char * in_id)
{
#if ENABLE_COMPONENT_IDS
	// free the existing id, if it isn't the empty string

	if (id[0] != 0)
		delete [] id;

	// if the new id is an empty string, avoid the new operation

	if (in_id == nullptr || in_id[0] == 0)
	{
		id = "";
	}
	else
	{
		// otherwise allocate some memory and copy the string

		const size_t length = strlen(in_id);

		id = new char[length + 1];
		memcpy((char*)id, in_id, length + 1);
	}
#endif
}

//

void ComponentSet::add(ComponentBase * component)
{
#if defined(DEBUG)
	// only one component of each unique type is allowed to exist in the component set
	for (auto * other = head; other != nullptr; other = other->next_in_set)
		Assert(other->typeIndex() != component->typeIndex());
#endif

	Assert(component->componentSet == nullptr);
	component->componentSet = this;
	
	component->next_in_set = head;
	head = component;
}

void ComponentSet::remove(ComponentBase * component)
{
	auto * itr = &head;
	
	for (;;)
	{
		Assert(*itr != nullptr);
		if (*itr == nullptr)
			break;
		
		if (*itr == component)
		{
			*itr = component->next_in_set;
			break;
		}
		
		itr = &(*itr)->next_in_set;
	}
	
	component->next_in_set = nullptr;
}

#if ENABLE_COMPONENT_IDS
ComponentBase * ComponentSet::find(const char * id)
{
	for (auto * component = head; component != nullptr; component = component->next_in_set)
	{
		if (strcmp(component->id, id) == 0)
			return component;
	}
	
	return nullptr;
}
#endif

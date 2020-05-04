#include "component.h"
#include <string.h>

//

static int s_nextComponentSetId = 0; // todo : reuse component set ids when freed, to avoid infinitely growing numbers

int allocComponentSetId()
{
	return s_nextComponentSetId++;
}

void freeComponentSetId(int & id)
{
	id = kComponentSetIdInvalid;
}

ComponentBase::~ComponentBase()
{
}

//

ComponentSet::ComponentSet()
{
	id = allocComponentSetId();
}

ComponentSet::~ComponentSet()
{
	freeComponentSetId(id);
	Assert(id == kComponentSetIdInvalid);
}

void ComponentSet::add(ComponentBase * component)
{
#if defined(DEBUG)
	// only one component of each unique type is allowed to exist in the component set
	for (auto * other = head; other != nullptr; other = other->next_in_set)
		Assert(other->typeIndex() != component->typeIndex());
#endif

	Assert(component->componentSet == nullptr);
	component->componentSet = this;
	
	ComponentBase ** tail = &head;
	
	while (*tail != nullptr)
		tail = &(*tail)->next_in_set;
	
	*tail = component;
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

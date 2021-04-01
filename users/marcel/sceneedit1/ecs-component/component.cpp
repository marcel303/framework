#include "component.h"
#include <vector>

// component set ids

// note : we reuse component set ids when freed, to avoid infinitely growing numbers
//        hence we define an array of freed component set ids, and a variable that
//        maintains the next (unique) id

static int s_nextComponentSetId = 0;

static std::vector<int> s_freedComponentSetIds;

int allocComponentSetId()
{
	int id;
	
	// reuse a freed component set id when possible
	
	if (!s_freedComponentSetIds.empty())
	{
		id = s_freedComponentSetIds.back();
		s_freedComponentSetIds.pop_back();
	}
	else
	{
		// if all component set ids are in use, allocate a new one
		
		id = s_nextComponentSetId++;
	}
	
	return id;
}

void freeComponentSetId(int & id)
{
	Assert(id != kComponentSetIdInvalid);
	
	s_freedComponentSetIds.push_back(id); // reuse the id
	
	id = kComponentSetIdInvalid;
}

// components

ComponentBase::~ComponentBase()
{
}

// component set

ComponentSet::ComponentSet()
{
	id = allocComponentSetId();
}

ComponentSet::~ComponentSet()
{
	Assert(head == nullptr);
	
	Assert(id != kComponentSetIdInvalid);
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

// -- component set helper functions

#include "componentType.h"
#include "componentTypeDB.h"

void freeComponentsInComponentSet(ComponentSet & componentSet)
{
	ComponentBase * next;

	for (auto * component = componentSet.head; component != nullptr; component = next)
	{
		// the component will be removed and next_in_set will become invalid, so we need to fetch it now
		
		next = component->next_in_set;
		
		auto * componentType = g_componentTypeDB.findComponentType(component->typeIndex());
		Assert(componentType != nullptr);
		
		auto * componentMgr = componentType->componentMgr;

		componentMgr->destroyComponent(componentSet.id);
		component = nullptr;
	}

	componentSet.head = nullptr;
}

void freeComponentInComponentSet(ComponentSet & componentSet, ComponentBase *& component)
{
	componentSet.remove(component);
	
	auto * componentType = g_componentTypeDB.findComponentType(component->typeIndex());
	Assert(componentType != nullptr);
	
	auto * componentMgr = componentType->componentMgr;
	Assert(componentMgr != nullptr);

	componentMgr->destroyComponent(componentSet.id);
	component = nullptr;
}

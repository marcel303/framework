#include "parameterComponent.h"
#include "parameterComponentUi.h"
#include "parameterUi.h"
#include <algorithm>

void doParameterUi(ParameterComponentMgr & componentMgr, const char * component_filter, const char * parameter_filter, const bool showAnonymousComponents)
{
	struct Elem
	{
		ParameterComponent * comp;
		
		bool operator<(const Elem & other) const
		{
			const int prefix_cmp = strcmp(comp->access_parameterMgr().access_prefix().c_str(), other.comp->access_parameterMgr().access_prefix().c_str());
			if (prefix_cmp != 0)
				return prefix_cmp < 0;
			
			const int id_cmp = strcmp(comp->id, other.comp->id);
			if (id_cmp != 0)
				return id_cmp < 0;
			
			return false;
		}
	};
	
	int numComponents = 0;
	
	for (auto * comp = componentMgr.head; comp != nullptr; comp = comp->next)
	{
		if (!comp->access_parameterMgr().access_parameters().empty())
			numComponents++;
	}
	
	Elem * elems = (Elem*)alloca(numComponents * sizeof(Elem));
	
	int numElems = 0;
	
	for (auto * comp = componentMgr.head; comp != nullptr; comp = comp->next)
		if (!comp->access_parameterMgr().access_parameters().empty())
			if (showAnonymousComponents || !comp->access_parameterMgr().access_prefix().empty())
					elems[numElems++].comp = comp;
	
	std::sort(elems, elems + numElems);
	
	for (int i = 0; i < numElems; ++i)
	{
		if (component_filter != nullptr && strcasestr(elems[i].comp->access_parameterMgr().access_prefix().c_str(), component_filter) == nullptr)
		{
			continue;
		}
		
		doParameterUi(elems[i].comp->access_parameterMgr(), parameter_filter);
	}
}

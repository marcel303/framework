#include "libgui_precompiled.h"
#include "Debugging.h"
#include "GuiPropertyList.h"

namespace Gui
{
	PropertyList::PropertyList()
	{
	}

	void PropertyList::Add(Property& property)
	{
		std::map<std::string, Owner>::iterator iterator;

		iterator = m_owners.find(property.GetOwnerClassName());

		if (iterator == m_owners.end())
		{
			Owner owner;
			m_owners[property.GetOwnerClassName()] = owner;
		}

		m_owners[property.GetOwnerClassName()].m_properties.push_back(property);
	}

	Property* PropertyList::Find(const std::string& ownerClassName, const std::string& name)
	{
		std::map<std::string, Owner>::iterator iterator;

		iterator = m_owners.find(ownerClassName);

		if (iterator == m_owners.end())
			return 0;

		Owner& owner = iterator->second;

		for (size_t i = 0; i < owner.m_properties.size(); ++i)
			if (owner.m_properties[i].GetOwnerClassName() == ownerClassName && owner.m_properties[i].GetName() == name)
				return &owner.m_properties[i];

		return 0;
	}

	void PropertyList::Serialize(IArchive& registry, int usageMask)
	{
		for (std::map<std::string, Owner>::iterator i = m_owners.begin(); i != m_owners.end(); ++i)
		{
			Owner& owner = i->second;

			for (size_t j = 0; j < owner.m_properties.size(); ++j)
				owner.m_properties[j].Serialize(registry, usageMask);
		}
	}

	void PropertyList::DeSerialize(IArchive& registry, int usageMask)
	{
		std::vector<std::string> keys = registry.GetKeys();

		for (std::map<std::string, Owner>::iterator i = m_owners.begin(); i != m_owners.end(); ++i)
		{
			Owner& owner = i->second;

			for (size_t j = 0; j < owner.m_properties.size(); ++j)
			{
				const std::string& name = owner.m_properties[j].GetName();

				bool found = false;

				for (size_t k = 0; k < keys.size(); ++k)
					if (keys[k] == name)
						found = true;

                if (found)
					owner.m_properties[j].DeSerialize(registry, usageMask);
			}
		}
	}
};

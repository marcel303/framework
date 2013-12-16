#pragma once

#include <map>
#include <vector>
#include "GuiProperty.h"
#include "IArchive.h"

namespace Gui
{
	class PropertyList
	{
	public:
		PropertyList();

		void Add(Property& property);

		Property* Find(const std::string& ownerClassName, const std::string& name);

		void Serialize(IArchive& registry, int usageMask);
		void DeSerialize(IArchive& registry, int usageMask);

	private:
		class Owner
		{
		public:
			std::vector<Property> m_properties;
		};

		std::map<std::string, Owner> m_owners;
	};
};

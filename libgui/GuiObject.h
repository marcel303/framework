#pragma once

#include "GuiEHNotify.h"
#include "GuiPropertyList.h"

#undef GetClassName // Windows compatibility fix.

namespace Gui
{
	class Object
	{
	public:
		typedef std::vector<std::string> ClassNameList;

		Object();
		virtual ~Object();

		const std::string& GetClassName() const;
		const ClassNameList& GetClassNameList() const;
		const std::string& GetName() const;

		void SetClassName(const std::string& className);
		void SetName(const std::string& name);

		bool IsA(const std::string& className);

		void AddProperty(Property property);

		void Serialize(IArchive& registry, int usageMask);
		void DeSerialize(IArchive& registry, int usageMask);

		DECLARE_EVENT(EHNotify, OnNameChangeBegin);
		DECLARE_EVENT(EHNotify, OnNameChange);

	private:
		std::string m_className;
		ClassNameList m_classNameList;
		std::string m_name;
		PropertyList m_propertyList;
	};
};

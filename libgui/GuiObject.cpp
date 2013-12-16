#include "libgui_precompiled.h"
#include "GuiObject.h"

namespace Gui
{
	Object::Object()
	{
		SetClassName("Object");

		AddProperty(Property("ClassName", m_className, Property::USAGE_BEHAVIOR));
		AddProperty(Property("Name",      m_name,      Property::USAGE_BEHAVIOR | Property::USAGE_DATA)); // NOTE: object name is associated with data, thus it needs to be serialize/deserialize when dealing with data as well.
	}

	Object::~Object()
	{
	}

	const std::string& Object::GetClassName() const
	{
		return m_className;
	}

	const Object::ClassNameList& Object::GetClassNameList() const
	{
		return m_classNameList;
	}

	const std::string& Object::GetName() const
	{
		return m_name;
	}

	void Object::SetClassName(const std::string& className)
	{
		m_className = className;

		m_classNameList.push_back(className);
	}

	void Object::SetName(const std::string& name)
	{
		if (name == m_name)
			return;

		DO_EVENT(OnNameChangeBegin, (this));

		m_name = name;

		DO_EVENT(OnNameChange, (this));
	}

	bool Object::IsA(const std::string& className)
	{
		for (size_t i = 0; i < m_classNameList.size(); ++i)
			if (m_classNameList[i] == className)
				return true;

		return false;
	}

	void Object::AddProperty(Property property)
	{
		if (property.GetOwnerClassName() == "")
			property.SetOwnerClassName(m_className);

		m_propertyList.Add(property);
	}

	void Object::Serialize(IArchive& registry, int usageMask)
	{
		m_propertyList.Serialize(registry, usageMask);
	}

	void Object::DeSerialize(IArchive& registry, int usageMask)
	{
		m_propertyList.DeSerialize(registry, usageMask);
	}
};

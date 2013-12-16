#include "libgui_precompiled.h"
#include "Debugging.h"
#include "GuiProperty.h"

namespace Gui
{
	Property::Property()
	{
	}

	Property::Property(const std::string& name, int& value, int usage)
	{
		Initialize(name, value, usage);
	}

	Property::Property(const std::string& name, float& value, int usage)
	{
		Initialize(name, value, usage);
	}

	Property::Property(const std::string& name, bool& value, int usage)
	{
		Initialize(name, value, usage);
	}

	Property::Property(const std::string& name, std::string& value, int usage)
	{
		Initialize(name, value, usage);
	}

	Property::Property(const std::string& name, Graphics::Image& value, int usage)
	{
		Initialize(name, value, usage);
	}

	/*Property::Property(const std::string& name, Graphics::Font& value, int usage)
	{
		Initialize(name, value, usage);
	}*/

	const std::string& Property::GetName()
	{
		return m_name;
	}

	const std::string& Property::GetOwnerClassName()
	{
		return m_ownerClassName;
	}

	void* Property::GetPointer()
	{
		return m_pointer;
	}

	int Property::GetUsage()
	{
		return m_usage;
	}

	Property::TYPE Property::GetType()
	{
		return m_type;
	}

	void Property::SetOwnerClassName(const std::string& ownerClassName)
	{
		m_ownerClassName = ownerClassName;
	}

	void Property::Serialize(IArchive& registry, int usageMask)
	{
		if (m_usage & usageMask)
			Serialize(registry);
	}

	void Property::DeSerialize(IArchive& registry, int usageMask)
	{
		if (m_usage & usageMask)
			DeSerialize(registry);
	}

	void Property::Serialize(IArchive& registry)
	{
		switch (m_type)
		{
		case TYPE_INT:
			registry[m_name] = *static_cast<int*>(m_pointer);
			break;
		case TYPE_FLOAT:
			registry[m_name] = *static_cast<float*>(m_pointer);
			break;
		case TYPE_BOOL:
			if (*static_cast<bool*>(m_pointer) == true)
				registry[m_name] = 1;
			else
				registry[m_name] = 0;
			break;
		case TYPE_STRING:
			registry[m_name] = *static_cast<std::string*>(m_pointer);
			break;
		case TYPE_IMAGE:
			{
				Graphics::Image* image = static_cast<Graphics::Image*>(m_pointer);
				registry[m_name] = image->GetName();
				bool transparent = image->IsTransparent();
				if (transparent)
					registry[m_name + "_Transparent"] = 1;
				else
					registry[m_name + "_Transparent"] = 0;
			}
			break;
		case TYPE_FONT:
			break;
		default:
			throw ExceptionVA("unknown property type: %d", m_type);
		}
	}

	void Property::DeSerialize(IArchive& registry)
	{
		switch (m_type)
		{
		case TYPE_INT:
			*static_cast<int*>(m_pointer) = registry(m_name, 0);
			break;
		case TYPE_FLOAT:
			*static_cast<float*>(m_pointer) = registry(m_name, 0.0f);
			break;
		case TYPE_BOOL:
			{
				int value = registry(m_name, 0);
				if (value != 0)
					*static_cast<bool*>(m_pointer) = true;
				else
					*static_cast<bool*>(m_pointer) = false;
			}
			break;
		case TYPE_STRING:
			*static_cast<std::string*>(m_pointer) = registry(m_name, "");
			break;
		case TYPE_IMAGE:
			{
				Graphics::Image* image = static_cast<Graphics::Image*>(m_pointer);
				std::string name = registry(m_name, "");
				bool isTransparent = registry(m_name + "_Transparent", 0) != 0;
				image->Setup(name.c_str(), isTransparent);
			}
			break;
		case TYPE_FONT:
			break;
		default:
			throw ExceptionVA("unknown property type: %d", m_type);
		}
	}

	void Property::Initialize(const std::string& name, int& value, int usage)
	{
		Initialize(name, &value, usage, TYPE_INT);
	}

	void Property::Initialize(const std::string& name, float& value, int usage)
	{
		Initialize(name, &value, usage, TYPE_FLOAT);
	}

	void Property::Initialize(const std::string& name, bool& value, int usage)
	{
		Initialize(name, &value, usage, TYPE_BOOL);
	}

	void Property::Initialize(const std::string& name, std::string& value, int usage)
	{
		Initialize(name, &value, usage, TYPE_STRING);
	}

	void Property::Initialize(const std::string& name, Graphics::Image& value, int usage)
	{
		Initialize(name, &value, usage, TYPE_IMAGE);
	}

	void Property::Initialize(const std::string& name, Graphics::Font& value, int usage)
	{
		Initialize(name, &value, usage, TYPE_FONT);
	}

	void Property::Initialize(const std::string& name, void* pointer, int usage, TYPE type)
	{
		m_name = name;
		m_pointer = pointer;
		m_usage = usage;
		m_type = type;
	}
};

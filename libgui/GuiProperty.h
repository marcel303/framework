#pragma once

#include <string>
#include "GuiGraphicsFont.h"
#include "GuiGraphicsImage.h"
#include "IArchive.h"

namespace Gui
{
	class Property
	{
	public:
		enum TYPE
		{
			TYPE_INT,
			TYPE_FLOAT,
			TYPE_BOOL,
			TYPE_STRING,
			TYPE_IMAGE,
			TYPE_FONT
		};

		enum USAGE
		{
			USAGE_BEHAVIOR = 0x01, // Behavioristic traits such as actions.
			USAGE_LAYOUT   = 0x02, // Layout info such as position, size, colors, etc.
			USAGE_DATA     = 0x04, // Sent during submit.
			USAGE_ANY      = 0xFF
		};

		Property();
		Property(const std::string& name, int& value, int usage);
		Property(const std::string& name, float& value, int usage);
		Property(const std::string& name, bool& value, int usage);
		Property(const std::string& name, std::string& value, int usage);
		Property(const std::string& name, Graphics::Image& value, int usage);
		Property(const std::string& name, Graphics::Font& value, int usage);

		const std::string& GetName();
		const std::string& GetOwnerClassName();
		void* GetPointer();
		int GetUsage();
		TYPE GetType();

		void SetOwnerClassName(const std::string& ownerClassName);

		void Serialize(IArchive& registry, int usageMask);
		void DeSerialize(IArchive& registry, int usageMask);
		void Serialize(IArchive& registry);
		void DeSerialize(IArchive& registry);

		void Initialize(const std::string& name, int& value, int usage);
		void Initialize(const std::string& name, float& value, int usage);
		void Initialize(const std::string& name, bool& value, int usage);
		void Initialize(const std::string& name, std::string& value, int usage);
		void Initialize(const std::string& name, Graphics::Image& value, int usage);
		void Initialize(const std::string& name, Graphics::Font& value, int usage);

	protected:
		void Initialize(const std::string& name, void* pointer, int usage, TYPE type);

		std::string m_ownerClassName;
		std::string m_name;
		void* m_pointer;
		int m_usage;
		TYPE m_type;
	};
};

#include "Options.h"

#if GG_ENABLE_OPTIONS

#include <stdio.h>
#include "FileStream.h"
#include "Log.h"
#include "Parse.h"
#include "StreamReader.h"
#include "StringEx.h" // _s variants

OptionManager g_optionManager;

// OptionManager

OptionBase * OptionManager::m_head = 0;

void OptionManager::Register(OptionBase * option)
{
	option->m_next = m_head;
	m_head = option;
}

void OptionManager::AddCommandOption(const char * path, OptionCommandHandlerWithParam handler, void * param)
{
	// fixme : currently leaks

	OptionCommandWithParam * option = new OptionCommandWithParam(path, handler, param);
}

OptionBase * OptionManager::FindOptionByPath(const char * path)
{
	for (OptionBase * option = m_head; option != 0; option = option->m_next)
		if (!strcmp(option->GetPath(), path))
			return option;

	return 0;
}

void OptionManager::Load(const char * filename)
{
	try
	{
		FileStream stream;
		stream.Open(filename, (OpenMode)(OpenMode_Read | OpenMode_Text));
		StreamReader reader(&stream, false);
		std::vector<std::string> lines = reader.ReadAllLines();
		for (auto l = lines.begin(); l != lines.end(); ++l)
		{
			std::string & line = *l;

			LoadFromString(line.c_str());
		}
	}
	catch (std::exception & e)
	{
		LOG_ERR("failed to load options from %s: %s", filename, e.what());
	}
}

void OptionManager::LoadFromString(const char * _line)
{
	std::string line = _line;

	size_t i = line.find('=');

	if (i != line.npos)
	{
		std::string name = line.substr(0, i);
		std::string value = line.substr(i + 1);

		for (OptionBase * option = m_head; option != 0; option = option->m_next)
		{
			if (!strcmp(option->m_name, name.c_str()) || (option->m_alias && !strcmp(option->m_alias, name.c_str())) || !strcmp(option->m_path, name.c_str()))
			{
				// look for a suitable alias

				const OptionValueAlias * alias;

				for (alias = option->m_valueAliasList; alias != 0; alias = alias->m_next)
					if (!strcmp(alias->m_alias, value.c_str()))
						break;

				option->FromString(alias ? alias->m_value : value.c_str());
			}
		}
	}
}

void OptionManager::LoadFromCommandLine(int argc, char * argv[])
{
	for (int i = 1; i < argc; ++i)
	{
		LoadFromString(argv[i]);
	}
}

// OptionBase

OptionBase::OptionBase(const char * path, const char * name)
	: m_next(0)
	, m_path(path)
	, m_name(name)
	, m_alias(0)
	, m_valueAliasList(0)
	, m_flags(0)
	, m_min(0.f)
	, m_max(0.f)
	, m_step(1.f)
{
	g_optionManager.Register(this);
}

OptionBase * OptionBase::GetNext() const
{
	return m_next;
}

const char * OptionBase::GetPath() const
{
	return m_path;
}

bool OptionBase::HasFlags(int flags) const
{
	return (m_flags & flags) == flags;
}

//

OptionCommand::OptionCommand(const char * path, OptionCommandHandler handler)
	: OptionBase(path, path)
	, m_handler(handler)
{
}

void OptionCommand::Select()
{
	m_handler();
}

//

OptionCommandWithParam::OptionCommandWithParam(const char * path, OptionCommandHandlerWithParam handler, void * param)
	: OptionBase(path, path)
	, m_handler(handler)
	, m_param(param)
{
}

void OptionCommandWithParam::Select()
{
	m_handler(m_param);
}

//

OptionValueAlias::OptionValueAlias(OptionBase & option, const char * alias, int value)
	: m_alias(alias)
{
	m_next = option.m_valueAliasList;
	option.m_valueAliasList = this;

	sprintf_s(m_value, sizeof(m_value), "%d", value);
}

// Option<bool>

template<> void Option<bool>::Increment()
{
	m_value = !m_value;
}

template<> void Option<bool>::Decrement()
{
	m_value = !m_value;
}

template<> void Option<bool>::ToString(char * buffer, int bufferSize)
{
	sprintf_s(buffer, bufferSize, "%d", m_value ? 1 : 0);
}

template<> void Option<bool>::FromString(const char * buffer)
{
	m_value = Parse::Bool(buffer);
}

// Option<int>

template<> void Option<int>::Increment()
{
	m_value += (int)m_step;

	ApplyLimits();
}

template<> void Option<int>::Decrement()
{
	m_value -= (int)m_step;

	ApplyLimits();
}

template<> void Option<int>::ToString(char * buffer, int bufferSize)
{
	sprintf_s(buffer, bufferSize, "%d", m_value);
}

template<> void Option<int>::FromString(const char * buffer)
{
	m_value = Parse::Int32(buffer);
}

// Option<float>

template<> void Option<float>::Increment()
{
	m_value += m_step;

	ApplyLimits();
}

template<> void Option<float>::Decrement()
{
	m_value -= m_step;

	ApplyLimits();
}

template<> void Option<float>::ToString(char * buffer, int bufferSize)
{
	sprintf_s(buffer, bufferSize, "%.3f", m_value);
}

template<> void Option<float>::FromString(const char * buffer)
{
	m_value = Parse::Float(buffer);
}

// Option<std::string>

template<> void Option<std::string>::Increment()
{
}

template<> void Option<std::string>::Decrement()
{
}

template<> void Option<std::string>::ToString(char * buffer, int bufferSize)
{
	strcpy_s(buffer, bufferSize, m_value.c_str());
}

template<> void Option<std::string>::FromString(const char * buffer)
{
	m_value = buffer;
}

#endif

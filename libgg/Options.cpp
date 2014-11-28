#include <stdio.h>
#include "FileStream.h"
#include "Log.h"
#include "Options.h"
#include "Parse.h"
#include "StreamReader.h"

#if GG_ENABLE_OPTIONS

OptionManager g_optionManager;

// OptionManager

OptionBase * OptionManager::m_head = 0;

void OptionManager::Register(OptionBase * option)
{
	option->m_next = m_head;
	m_head = option;
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

			size_t i = line.find('=');

			if (i != line.npos)
			{
				std::string name = line.substr(0, i);
				std::string value = line.substr(i + 1);

				for (OptionBase * option = m_head; option != 0; option = option->m_next)
				{
					if (!strcmp(option->m_name, name.c_str()) || !strcmp(option->m_path, name.c_str()))
					{
						option->FromString(value.c_str());
					}
				}
			}
		}
	}
	catch (std::exception & e)
	{
		LOG_ERR("failed to load options from %s: %s", filename, e.what());
	}
}

// OptionBase

OptionBase::OptionBase(const char * path, const char * name)
	: m_next(0)
	, m_path(path)
	, m_name(name)
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

// Option<bool>

void Option<bool>::Increment()
{
	m_value = !m_value;
}

void Option<bool>::Decrement()
{
	m_value = !m_value;
}

void Option<bool>::ToString(char * buffer, int bufferSize)
{
	sprintf_s(buffer, bufferSize, "%d", m_value ? 1 : 0);
}

void Option<bool>::FromString(const char * buffer)
{
	m_value = Parse::Bool(buffer);
}

// Option<int>

void Option<int>::Increment()
{
	m_value += (int)m_step;

	ApplyLimits();
}

void Option<int>::Decrement()
{
	m_value -= (int)m_step;

	ApplyLimits();
}

void Option<int>::ToString(char * buffer, int bufferSize)
{
	sprintf_s(buffer, bufferSize, "%d", m_value);
}

void Option<int>::FromString(const char * buffer)
{
	m_value = Parse::Int32(buffer);
}

// Option<float>

void Option<float>::Increment()
{
	m_value += m_step;

	ApplyLimits();
}

void Option<float>::Decrement()
{
	m_value -= m_step;

	ApplyLimits();
}

void Option<float>::ToString(char * buffer, int bufferSize)
{
	sprintf_s(buffer, bufferSize, "%.3f", m_value);
}

void Option<float>::FromString(const char * buffer)
{
	m_value = Parse::Float(buffer);
}

#endif

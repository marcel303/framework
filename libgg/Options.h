#pragma once

#define GG_ENABLE_OPTIONS 1

#if GG_ENABLE_OPTIONS

class OptionBase
{
	friend class OptionAlias;
	friend class OptionLimits;
	friend class OptionManager;

	OptionBase * m_next;
	const char * m_path;
	const char * m_name;
	const char * m_alias;

protected:
	float m_min;
	float m_max;
	float m_step;

	OptionBase(const char * path, const char * name);

public:
	OptionBase * GetNext() const;
	const char * GetPath() const;

	virtual void Increment() = 0;
	virtual void Decrement() = 0;
	virtual void ToString(char * buffer, int bufferSize) = 0;
	virtual void FromString(const char * buffer) = 0;
};

template <typename T>
class Option : public OptionBase
{
	T m_value;

protected:
	void ApplyLimits()
	{
		if (m_min == 0.f && m_max == 0.f)
			return;
		if (m_value < (T)m_min)
			m_value = (T)m_min;
		if (m_value > (T)m_max)
			m_value = (T)m_max;
	}

public:
	typedef T Type;

	Option(const T & value, const char * path, const char * name)
		: OptionBase(path, name)
		, m_value(value)
	{
	}

	virtual void Increment();
	virtual void Decrement();
	virtual void ToString(char * buffer, int bufferSize);
	virtual void FromString(const char * buffer);

	//

	operator T() const
	{
		return m_value;
	}

	T & operator=(const T & value)
	{
		m_value = value;
	}
};

class OptionLimits
{
public:
	OptionLimits(OptionBase & option, float min, float max, float step)
	{
		option.m_min = min;
		option.m_max = max;
		option.m_step = step;
	}
};

class OptionAlias
{
public:
	OptionAlias(OptionBase & option, const char * alias)
	{
		option.m_alias = alias;
	}
};

class OptionManager
{
public:
	static OptionBase * m_head;

	void Register(OptionBase * option);
	void Load(const char * filename);
	void LoadFromString(const char * line);
	void LoadFromCommandLine(int argc, char * argv[]);
};

extern OptionManager g_optionManager;

#define OPTION_EXTERN(type, name) extern Option<type> name
#define OPTION_DECLARE(type, name, defaultValue) OPTION_EXTERN(type, name); static const type name ## _defaultValue = defaultValue
#define OPTION_DEFINE(type, name, path) Option<type> name(name ## _defaultValue, path, # name)
#define OPTION_STEP(name, min, max, step) static OptionLimits name ## _limits(name, min, max, step)
#define OPTION_ALIAS(name, alias) static OptionAlias name ## _alias(name, alias)

#else

#define OPTION_EXTERN(type, name) extern type name
#define OPTION_DECLARE(type, name, defaultValue) OPTION_EXTERN(type, name); static const type name ## _defaultValue = defaultValue
#define OPTION_DEFINE(type, name, path) type name(name ## _defaultValue)
#define OPTION_STEP(name, min, max, step)

#endif

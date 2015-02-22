#pragma once

#define GG_ENABLE_OPTIONS 1

#if GG_ENABLE_OPTIONS

class OptionBase
{
	friend class OptionAlias;
	friend class OptionFlags;
	friend class OptionLimits;
	friend class OptionManager;
	friend class OptionValueAlias;

	OptionBase * m_next;
	const char * m_path;
	const char * m_name;
	const char * m_alias;
	OptionValueAlias * m_valueAliasList;
	int m_flags;

protected:
	float m_min;
	float m_max;
	float m_step;

	OptionBase(const char * path, const char * name);

public:
	enum OptionType
	{
		kType_Value,
		kType_Command
	};

	OptionBase * GetNext() const;
	const char * GetPath() const;
	const OptionValueAlias * GetValueAliasList() const { return m_valueAliasList; }
	bool HasFlags(int flag) const;

	virtual OptionType GetType() = 0;
	virtual void Increment() = 0;
	virtual void Decrement() = 0;
	virtual void Select() = 0;
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

	virtual OptionType GetType() { return kType_Value; }
	virtual void Increment();
	virtual void Decrement();
	virtual void Select() { }
	virtual void ToString(char * buffer, int bufferSize);
	virtual void FromString(const char * buffer);

	//

	operator T() const
	{
		return m_value;
	}

	void operator=(const T & value)
	{
		m_value = value;
	}
};

//

typedef void (*OptionCommandHandler)();
typedef void (*OptionCommandHandlerWithParam)(void * param);

class OptionCommand : public OptionBase
{
	OptionCommandHandler m_handler;

public:
	OptionCommand(const char * path, OptionCommandHandler handler);

	virtual OptionType GetType() { return kType_Command; }
	virtual void Increment() { }
	virtual void Decrement() { }
	virtual void Select();
	virtual void ToString(char * buffer, int bufferSize) { buffer[0] = 0; }
	virtual void FromString(const char * buffer) { }
};

class OptionCommandWithParam : public OptionBase
{
	OptionCommandHandlerWithParam m_handler;
	void * m_param;

public:
	OptionCommandWithParam(const char * path, OptionCommandHandlerWithParam handler, void * param);

	virtual OptionType GetType() { return kType_Command; }
	virtual void Increment() { }
	virtual void Decrement() { }
	virtual void Select();
	virtual void ToString(char * buffer, int bufferSize) { buffer[0] = 0; }
	virtual void FromString(const char * buffer) { }
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

class OptionValueAlias
{
	friend class OptionManager;

	static const int kMaxValueSize = 64;

	OptionValueAlias * m_next;
	const char * m_alias;
	char m_value[kMaxValueSize];

public:
	OptionValueAlias(OptionBase & option, const char * alias, int value);

	const OptionValueAlias * GetNext() const { return m_next; }
	const char * GetAlias() const { return m_alias; }
	const char * GetValue() const { return m_value; }
};

class OptionFlags
{
public:
	OptionFlags(OptionBase & option, int flags)
	{
		option.m_flags = flags;
	}
};

class OptionManager
{
public:
	static OptionBase * m_head;

	void Register(OptionBase * option);

	void AddCommandOption(const char * path, OptionCommandHandlerWithParam handler, void * param);

	OptionBase * FindOptionByPath(const char * path);

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
#define OPTION_VALUE_ALIAS(name, alias, value) static OptionValueAlias name ## _alias_ ## alias(name, # alias, value)
#define OPTION_FLAGS(name, flags) static OptionFlags name ## _flags(name, flags)
#define COMMAND_OPTION(name, path, command) static OptionCommand name(path, command)

#define OPTION_FLAG_HIDDEN (1 << 0)

#else

#define OPTION_EXTERN(type, name) extern type name
#define OPTION_DECLARE(type, name, defaultValue) OPTION_EXTERN(type, name); static const type name ## _defaultValue = defaultValue
#define OPTION_DEFINE(type, name, path) type name(name ## _defaultValue)
#define OPTION_STEP(name, min, max, step)
#define OPTION_ALIAS(name, alias)
#define OPTION_VALUE_ALIAS(name, alias)
#define OPTION_FLAGS(name, flags)
#define COMMAND_OPTION(name, command, path)

#endif

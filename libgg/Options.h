#pragma once

#define GG_ENABLE_OPTIONS 1

#if GG_ENABLE_OPTIONS

class OptionBase
{
	friend class OptionManager;

	OptionBase * m_next;
	const char * m_path;
	const char * m_name;

protected:
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

class OptionManager
{
public:
	static OptionBase * m_head;

	void Register(OptionBase * option);
	void Load(const char * filename);
};

extern OptionManager g_optionManager;

#define OPTION_EXTERN(type, name) extern Option<type> name
#define OPTION_DECLARE(type, name, defaultValue) OPTION_EXTERN(type, name); static const type name ## _defaultValue = defaultValue
#define OPTION_DEFINE(type, name, path) Option<type> name(name ## _defaultValue, path, # name)

#else

#define OPTION_EXTERN(type, name) extern type name
#define OPTION_DECLARE(type, name, defaultValue) OPTION_EXTERN(type, name); static const type name ## _defaultValue = defaultValue
#define OPTION_DEFINE(type, name, path) type name(name ## _defaultValue)

#endif

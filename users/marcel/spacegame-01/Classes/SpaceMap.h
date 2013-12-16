#pragma once

// Immutable string type.
// todo: free copy.
class String
{
public:
	String();
	String(const char* str);
	String(const String& str);
	
	int Length_get();
	const char* cstr();
	
	String operator+(const String& str);
	
	static String& Empty_get();
	
private:
	char* m_Value;
};

// Base object type.
class IObject
{
public:
	virtual bool IsEqual(IObject* obj);
	virtual int GetHash() = 0;
	virtual String ToString()
	{
		return String.Empty_get();
	}
};

template <typename T, typename U>
class Dictionary<T, U>;

template <typename T>
class List<T>;

class SpaceObject
{
public:
};

class SpaceMap
{
public:
	SpaceMap()
	{
		Initialize();
	}
	
	void Initialize()
	{
	}
	
	List<SpaceObject> m_Objects;
};

class Vec2;
class Vec3;
class Vec4;

class SpaceDebris : public SpaceObject
{
public:
};

class SpaceShip : public SpaceObject
{
public:
};

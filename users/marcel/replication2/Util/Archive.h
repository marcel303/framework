#ifndef ARCHIVE_H
#define ARCHIVE_H
#pragma once

#include <map>
#include <string>
#include "libgg_forward.h"
#include "ParseTokenizer.h"

// Supported types.
#include "Mat4x4.h"
#include "Types.h"
#include "Vec2.h"
#include "Vec3.h"
#include "Vec4.h"

class ArKey
{
public:
	ArKey()
	{
		m_assigned = false;
	}
	ArKey(const std::string& name)
	{
		m_name = name;
		m_assigned = false;
	}

	std::string m_name;
	std::string m_value;
	bool m_assigned;
};

class ArRecordID
{
public:
	ArRecordID()
	{
		m_index = 0;
	}

	ArRecordID(const std::string& name, int index)
	{
		m_name = name;
		m_index = index;
	}

	std::string m_name;
	int m_index;

	bool operator<(const ArRecordID& id) const
	{
		if (m_name < id.m_name)
			return true;
		if (m_name > id.m_name)
			return false;
		if (m_index < id.m_index)
			return true;
		if (m_index > id.m_index)
			return false;

		return false;
	}
};

class ArRecord
{
public:
	typedef std::map<std::string, ArKey*> KeyMap;
	typedef KeyMap::iterator KeyItr;
	typedef std::map<ArRecordID, ArRecord*> RecordMap;
	typedef RecordMap::iterator RecordItr;

	ArRecord()
	{
		m_parent = 0;
	}

	~ArRecord()
	{
		for (KeyItr i = m_keys.begin(); i != m_keys.end(); ++i)
			delete i->second;

		for (RecordItr i = m_records.begin(); i != m_records.end(); ++i)
			delete i->second;
	}

	int GetDepth()
	{
		int depth = 0;

		ArRecord* temp = this;

		while ((temp = temp->m_parent))
			++depth;

		return depth;
	}

	bool KeyExists(const std::string& name)
	{
		return m_keys.count(name) != 0;		
	}

	bool RecordExists(const std::string& name, int index = 0)
	{
		ArRecordID id(name, index);

		return m_records.count(id) != 0;		
	}

	KeyItr FindKey(const std::string& name)
	{
		return m_keys.find(name);
	}

	RecordItr FindRecord(ArRecordID& id)
	{
		return m_records.find(id);
	}

	ArKey* AddKey(const std::string& name)
	{
		ArKey* key = new ArKey(name);

		m_keys[name] = key;

		return key;
	}

	ArRecord* AddRecord(ArRecordID& id)
	{
		ArRecord* record = new ArRecord();
		record->m_parent = this;
		record->m_id = id;
		m_records[id] = record;

		return record;
	}

	ArRecord* m_parent;
	ArRecordID m_id;
	KeyMap m_keys;
	RecordMap m_records;
};

class Ar
{
public:
	Ar();
	~Ar();

	void Clear();

	bool Read(const std::string& filename);
	bool Read(StreamReader& reader);
	bool ReadFromString(const std::string& text);
	bool Write(const std::string& filename) const;
	bool Write(StreamWriter& writer) const;
	bool WriteToString(std::string& out_text) const;

	void Begin(const std::string& name, int index = 0);
	void End();
	bool Exists(const std::string& name, int index = 0) const;

	bool Merge(Ar& ar);
	bool Merge(const std::string& filename);
	bool MergeFromString(const std::string& text);

	const std::vector<std::string> GetKeys()
	{
		std::vector<std::string> result;

		ArRecord::KeyMap keys = m_currRecord->m_keys;

		for (ArRecord::KeyMap::iterator i = keys.begin(); i != keys.end(); ++i)
			result.push_back(i->first);

		return result;
	}

	Ar& operator[](const std::string& key);

	// Read.
	#define default _default
	int operator()(const std::string& key, int default) const;
	float operator()(const std::string& key, float default) const;
	std::string operator()(const std::string& key, const std::string& default) const;
	Vec3 operator()(const std::string& key, const Vec3& default) const;
	Vec4 operator()(const std::string& key, const Vec4& default) const;
	Mat4x4 operator()(const std::string& key, const Mat4x4& default) const;
	#undef default

	// Write.
	Ar& operator=(int v);
	Ar& operator=(uint32_t v);
	Ar& operator=(float v);
	Ar& operator=(const std::string& v);
	Ar& operator=(const Vec3& v);
	Ar& operator=(const Vec4& v);
	Ar& operator=(const Mat4x4& v);

public:
	bool WriteText(const std::string& text, int depth, std::string& out_text);
	bool WriteRecord(bool root, ArRecord* record, int depth, std::string& out_text);
	bool ParseRecord(bool root, Parse::Tokenizer& tk, int& offset);

	void GotoTop();

	bool Merge(ArRecord* record);

	void Key_Begin(const std::string& key);
	bool Key_IsSet(const std::string& key) const;
	const std::string& Key_Get(const std::string& key, const std::string& _default) const;
	void Key_Set(const std::string& key, const std::string& value);
	
	ArRecord* m_root;
	ArRecord* m_currRecord;
	ArKey* m_currKey;
};

#endif

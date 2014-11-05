#include <stack>
#include "Archive.h"
#include "Array.h"
#include "Debug.h"
#include "FileStream.h"
#include "StreamReader.h"
#include "StreamWriter.h"
//#include "Quaternion.h"

enum Tokens
{
	TK_SECTION_IDENTIFIER_L = Parse::TK_CUSTOM,
	TK_SECTION_IDENTIFIER_R = Parse::TK_CUSTOM,
	TK_BRACE_L,
	TK_BRACE_R,
	TK_EQUALS
};

Ar::Ar()
{
	m_root = 0;
	
	Clear();
}

Ar::~Ar()
{
	delete m_root;
}

void Ar::Clear()
{
	if (m_root)
		delete m_root;
	
	m_root = new ArRecord();
	
	m_root->m_id = ArRecordID("root", 0);
	
	GotoTop();	
}

bool Ar::Read(const std::string& filename)
{
	FileStream stream;

	try
	{
		stream.Open(filename.c_str(), OpenMode_Read);
	}
	catch (std::exception&)
	{
		//SetErrorString("Unable to read from text file (" + f.m_filename + ").");
		return false;
	}

	StreamReader reader(&stream, false);

	return Read(reader);
}

bool Ar::Read(StreamReader& reader)
{
	std::string text = reader.ReadAllText();

	return ReadFromString(text);
}

bool Ar::ReadFromString(const std::string& text)
{
	Clear();
	
	Parse::Tokenizer tokenizer;
	
	const Parse::Token tokenArray[] =
	{
		Parse::Token(0, TK_SECTION_IDENTIFIER_L, "["),
		Parse::Token(0, TK_SECTION_IDENTIFIER_R, "]"),
		Parse::Token(0, TK_BRACE_L, "{"),
		Parse::Token(0, TK_BRACE_R, "}"),
		Parse::Token(0, TK_EQUALS, "="),
		Parse::Token(0, Parse::TK_ZERO, "")
	};
	
	tokenizer.SetOption(Parse::Tokenizer::OPTION_CHECK_LITERAL_COLLISIONS, 0);
	
	Parse::String parseString = Parse::String(text);

	if (!tokenizer.Tokenize(parseString, tokenArray))
	{
		//SetErrorString(std::string("Unable to tokenize: ") + tokenizer.GetErrorMessage());
		return false;
	}
	
	int offset = 0;
	
	bool ret = ParseRecord(true, tokenizer, offset);

	if (!ret)
	{
		//SetErrorString("Unable to parse.");
		return false;
	}
	
	return true;
}

bool Ar::Write(const std::string& filename) const
{
	FileStream stream;
	
	try
	{
		stream.Open(filename.c_str(), static_cast<OpenMode>(OpenMode_Write | OpenMode_Text));
	}
	catch (std::exception&)
	{
		//_this->SetErrorString("Unable to open file for writing (" + filename + ").");
		return false;
	}
	
	StreamWriter writer(&stream, false);

	return Write(writer);
}

bool Ar::Write(StreamWriter& writer) const
{
	std::string text;

	if (!WriteToString(text))
		return false;

	try
	{
		writer.WriteText(text);
	}
	catch (std::exception&)
	{
		return false;
	}

	return true;
}

bool Ar::WriteToString(std::string& out_text) const
{
	Ar* _this = const_cast<Ar*>(this);

	if (!_this->WriteRecord(true, m_root, 0, out_text))
		return false;

	return true;
}

void Ar::Begin(const std::string& name, int index)
{
	Assert(m_currRecord);
	
	ArRecordID id(name, index);
	
	ArRecord::RecordItr i =	m_currRecord->FindRecord(id);
	
	if (i == m_currRecord->m_records.end())
	{
		ArRecord* record = m_currRecord->AddRecord(id);

		m_currRecord = record;
	}
	else
	{
		m_currRecord = i->second;
	}
}

void Ar::End()
{
	Assert(m_currRecord);
	Assert(m_currRecord->m_parent);
	
	m_currRecord = m_currRecord->m_parent;
}

bool Ar::Exists(const std::string& name, int index) const
{
	return m_currRecord->RecordExists(name, index);
}

bool Ar::Merge(Ar& ar)
{
	return Merge(ar.m_root);
}

bool Ar::Merge(const std::string& filename)
{
	Ar ar;

	if (!ar.Read(filename))
		return false;

	return Merge(ar);
}

bool Ar::MergeFromString(const std::string& text)
{
	Ar ar;

	if (!ar.ReadFromString(text))
		return false;

	return Merge(ar);
}

Ar& Ar::operator[](const std::string& key)
{
	Key_Begin(key);
	
	return *this;
}

int Ar::operator()(const std::string& key, int _default) const
{
	if (!Key_IsSet(key))
	{
		//Debug::VerboseMessage("Attempt to read non existing key value (%s).", key.c_str());
		return _default;
	}

	std::string string = Key_Get(key, "");
	
	Parse::Tokenizer tk;
	
	Parse::String parseString = Parse::String(string);

	if (!tk.Tokenize(parseString, 0))
	{
		//SetErrorString("Unable to tokenize input string.");
		return _default;
	}
	
	if (tk.GetTokenCount() != 1)
	{
		//SetErrorString("Key doesn't contain the right number of elements to be an integer.");
		return _default;
	}
	
	if (tk.GetToken(0)->token != Parse::TK_INTEGER)
	{
		//SetErrorString("Key isn't an integer.");
		return _default;
	}
	
	return tk.GetToken(0)->vI;
		
}

float Ar::operator()(const std::string& key, float _default) const
{
	if (!Key_IsSet(key))
	{
		//Debug::VerboseMessage("Attempt to read non existing key value (%s).", key.c_str());
		return _default;
	}

	std::string string = Key_Get(key, "");
	
	Parse::Tokenizer tokenizer;
	
	Parse::String parseString = Parse::String(string);

	if (!tokenizer.Tokenize(parseString, 0))
	{
		//SetErrorString("Unable to tokenize input string.");
		return _default;
	}
	
	if (tokenizer.GetTokenCount() != 1)
	{
		//SetErrorString("Key doesn't contain the right number of elements to be a float.");
		return _default;
	}
	
	if (tokenizer.GetToken(0)->token != Parse::TK_FLOAT && tokenizer.GetToken(0)->token != Parse::TK_INTEGER)
	{
		//SetErrorString("Key isn't a float or integer.");
		return _default;
	}
	
	if (tokenizer.GetToken(0)->token == Parse::TK_FLOAT)
		return tokenizer.GetToken(0)->vF;
	else
		return static_cast<float>(tokenizer.GetToken(0)->vI);
}

std::string Ar::operator()(const std::string& key, const std::string& _default) const
{
	if (!Key_IsSet(key))
	{
		//Debug::VerboseMessage("Attempt to read non existing key value (%s).", key.c_str());
		return _default;
	}

	return Key_Get(key, _default);

}

Vec4 Ar::operator()(const std::string& key, const Vec4& _default) const
{
	if (!Key_IsSet(key))
	{
		//Debug::VerboseMessage("Attempt to read non existing key value (%s).", key.c_str());
		return _default;
	}
	
	std::string string = (*this)(key, "");
	
	// Try to decode string to 3 or 4 floats.
	
	Parse::Tokenizer tokenizer;
	
	Parse::String parseString = Parse::String(string);

	if (!tokenizer.Tokenize(parseString, 0))
	{
		//SetErrorString(std::string("Unable to tokenize input string ") + string + ".");
		return _default;
	}
	
	if (tokenizer.GetTokenCount() != 3 && tokenizer.GetTokenCount() != 4)
	{
		//SetErrorString("Key doesn't contain enough elements to be a 3D or 4D vector.");
		return _default;
	}
	
	// Check if all elements are float / integer & convert.
	
	float v[4];
	
	for (int i = 0; i < tokenizer.GetTokenCount(); ++i)
	{
		const Parse::Token* token = tokenizer.GetToken(i);
		if (token->token == Parse::TK_FLOAT)
		{
			v[i] = token->vF;
		}
		else if (token->token == Parse::TK_INTEGER)
		{
			v[i] = (float)token->vI;
		}
		else
		{
			//SetErrorString("Key isn't a vector.");
			return _default;
		}
	}
	
	// Convert elements to vector.
	
	Vec4 vector;
	
	for (int i = 0; i < tokenizer.GetTokenCount(); ++i)
	{
		vector[(int)i] = v[i];
	}
	
	return vector;
	
}

Mat4x4 Ar::operator()(const std::string& key, const Mat4x4& _default) const
{
	if (!Key_IsSet(key))
	{
		return _default;
	}
	
	const std::string string = (*this)(key, "");
		
	// Try to decode string to an array of floats.
	
	Parse::Tokenizer tokenizer;
	
	Parse::String parseString = Parse::String(string);

	if (!tokenizer.Tokenize(parseString, 0))
	{
		//SetErrorString(std::string("Unable to tokenize input string ") + string + ".");
		return _default;
	}
	
	if (
		tokenizer.GetTokenCount() != 3 && // Translation.
		tokenizer.GetTokenCount() != 4 && // Axis-angle rotation.
		tokenizer.GetTokenCount() != 6 && // Translation + euler rotation.
		tokenizer.GetTokenCount() != 7 && // Translation + axis-angle rotation.
		tokenizer.GetTokenCount() != 9 && // 3x3 rotation matrix.
		tokenizer.GetTokenCount() != 16) // 4x4 rotation matrix.
	{
		//SetErrorString("Key doesn't contain enough elements to be a 3x3 or 4x4 matrix.");
		return _default;
	}
	
	// Check if all elements are float / integer & convert.
	
	float v[16];
	
	for (int i = 0; i < tokenizer.GetTokenCount(); ++i)
	{
		const Parse::Token* token = tokenizer.GetToken(i);
		if (token->token == Parse::TK_FLOAT)
		{
			v[i] = token->vF;
		}
		else if (token->token == Parse::TK_INTEGER)
		{
			v[i] = (float)token->vI;
		}
		else
		{
			//SetErrorString("Key isn't a vector.");
			return _default;
		}
	}
		
	// Convert elements to matrix.
	
	Mat4x4 matrix;
	
	if (tokenizer.GetTokenCount() == 3)
	{
		// Translation.
		matrix.MakeTranslation(v[0], v[1], v[2]);
	}
	/*
	else if (tokenizer.GetTokenCount() == 4)
	{
		// Axis-angle pair.
		Quaternion quaternion;
		quaternion.FromAxisAngle(Vector(v[0], v[1], v[2]), v[3]);
		matrix = quaternion.ToMatrix();
	}*/
	/*
	else if (tokenizer.GetTokenCount() == 6)
	{
		// Translation + euler rotation.
		Mat4x4 t;
		Mat4x4 r;
		t.MakeTranslation(Vector(v[0], v[1], v[2]));
		r.MakeRotationEuler(Vector(v[3], v[4], v[5]));
		matrix = t * r;
	}*/
	/*
	else if (tokenizer.GetTokenCount() == 7)
	{
		// Translation + axis-angle pair.
		Mat4x4 t;
		Mat4x4 r;
		t.MakeTranslation(Vector(v[0], v[1], v[2]));
		Quaternion quaternion;
		quaternion.FromAxisAngle(Vector(v[3], v[4], v[5]), v[6]);
		r = quaternion.ToMatrix();
		matrix = t * r;
	}*/
	else if (tokenizer.GetTokenCount() == 9)
	{
		// Rotation matrix.

		// Row 1.
		matrix(0, 0) = v[0];
		matrix(1, 0) = v[1];
		matrix(2, 0) = v[2];
		matrix(3, 0) = 0.0f;
		// Row 2.
		matrix(0, 1) = v[3];
		matrix(1, 1) = v[4];
		matrix(2, 1) = v[5];
		matrix(3, 1) = 0.0f;
		// Row 3.
		matrix(0, 2) = v[6];
		matrix(1, 2) = v[7];
		matrix(2, 2) = v[8];
		matrix(3, 2) = 0.0f;
		// Row 4.
		matrix(0, 3) = 0.0f;
		matrix(1, 3) = 0.0f;
		matrix(2, 3) = 0.0f;
		matrix(3, 3) = 1.0f;
	}
	else
	{
		// 4x4 transformation matrix.

		// Row 1.
		matrix(0, 0) = v[0];
		matrix(1, 0) = v[1];
		matrix(2, 0) = v[2];
		matrix(3, 0) = v[3];
		// Row 2.
		matrix(0, 1) = v[4];
		matrix(1, 1) = v[5];
		matrix(2, 1) = v[6];
		matrix(3, 1) = v[7];
		// Row 3.
		matrix(0, 2) = v[8];
		matrix(1, 2) = v[9];
		matrix(2, 2) = v[10];
		matrix(3, 2) = v[11];
		// Row 4.
		matrix(0, 3) = v[12];
		matrix(1, 3) = v[13];
		matrix(2, 3) = v[14];
		matrix(3, 3) = v[15];
	}
	
	return matrix;
}

Ar& Ar::operator=(int v)
{
	Assert(m_currKey);
	
	char temp[100];
	sprintf(temp, "%d", v);
	
	Key_Set(m_currKey->m_name, temp);
	
	return (*this);
	
}

Ar& Ar::operator=(uint32_t v)
{
	Assert(m_currKey);
	
	char temp[100];
	sprintf(temp, "%d", (int)v);
	
	Key_Set(m_currKey->m_name, temp);
	
	return (*this);
	
}

Ar& Ar::operator=(float v)
{
	Assert(m_currKey);
	
	char temp[100];
	sprintf(temp, "%f", v);
	
	Key_Set(m_currKey->m_name, temp);

	return (*this);
	
}

Ar& Ar::operator=(const std::string& v)
{
	Assert(m_currKey);
	
	Key_Set(m_currKey->m_name, v);
	
	return (*this);
	
}

Ar& Ar::operator=(const Vec4& vector)
{
	Assert(m_currKey);
	
	char temp[256];
	sprintf(temp, "%f %f %f %f",
		vector[0],
		vector[1],
		vector[2],
		vector[3]);
	
	Key_Set(m_currKey->m_name, temp);
	
	return (*this);
	
}

Ar& Ar::operator=(const Mat4x4& matrix)
{
	Assert(m_currKey);
	
	char temp[1024];
	sprintf(temp, "%f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f",
		// Row 1.
		matrix(0, 0),
		matrix(1, 0),
		matrix(2, 0),
		matrix(3, 0),
		// Row 2.
		matrix(0, 1),
		matrix(1, 1),
		matrix(2, 1),
		matrix(3, 1),
		// Row 3.
		matrix(0, 2),
		matrix(1, 2),
		matrix(2, 2),
		matrix(3, 2),
		// Row 4.
		matrix(0, 3),
		matrix(1, 3),
		matrix(2, 3),
		matrix(3, 3));
	
	Key_Set(m_currKey->m_name, temp);
	
	return (*this);
	
}

bool Ar::WriteText(const std::string& text, int depth, std::string& out_text)
{
	for (int i = 0; i < depth; ++i)
		out_text.push_back('\t');
	
	out_text += text;

	return true;
}

bool Ar::WriteRecord(bool root, ArRecord* record, int depth, std::string& out_text)
{
	if (!root)
	{
		WriteText("[" + record->m_id.m_name + "]\n", depth, out_text);
		WriteText("{\n", depth, out_text);
		++depth;
		WriteText("\n", depth, out_text);
	}
	
	// Write keys.
	
	if (record->m_keys.size() > 0)
	{
		for (ArRecord::KeyItr i = record->m_keys.begin(); i != record->m_keys.end(); ++i)
			if (i->second->m_assigned)
				WriteText(i->second->m_name + " = \"" + i->second->m_value + "\"\n", depth, out_text);
		
		WriteText("\n", depth, out_text);
	}
	
	// Write children.
	
	if (record->m_records.size() > 0)
	{
		for (ArRecord::RecordItr i = record->m_records.begin(); i != record->m_records.end(); ++i)
		{
			if (!WriteRecord(false, i->second, depth, out_text))
				return false;
		}

		WriteText("\n", depth, out_text);
	}
	
	if (!root)
	{
		--depth;
		WriteText("}\n", depth, out_text);
	}

	return true;
}

bool Ar::ParseRecord(bool root, Parse::Tokenizer& tk, int& offset)
{
	std::string recordName;

	if (!root)
	{
		// Check for '['.
		if (tk[offset]->token != TK_SECTION_IDENTIFIER_L)
			return false;
		
		++offset;
		
		// Check for record name.
		if (tk[offset]->token != Parse::TK_STRING)
			return false;

		// Read record name.
		recordName = (const char*)tk.GetToken(offset)->string;

		++offset;

		// Check for ']'.
		if (tk.GetToken(offset)->token != TK_SECTION_IDENTIFIER_R)
			return false;
		
		++offset;

		// Check for '{'.
		if (tk.GetToken(offset)->token != TK_BRACE_L)
			return false;
		
		++offset;
	}
	else
	{
		recordName = "root";
	}
	
	if (recordName != "root")
	{
		// Find unique tag.
		int index = 0;
		
		bool bContinue = true;
		
		for (int i = 0; bContinue; ++i)
		{
			if (!Exists(recordName, i))
			{
				index = i;
				bContinue = false;
			}
		}
		
		// Begin record.
		Begin(recordName, index);
	}
	
	// Read keys & records.
	
	// Read until '}'.
	while (tk.GetToken(offset)->token != TK_BRACE_R && tk.GetToken(offset)->token != Parse::TK_ZERO)
	{
		// Check for EOF.
		//if (tk.GetToken(offset)->token == Parse::TK_ZERO)
			//return false;
		
		// Check for key.
		if (tk.GetToken(offset)->token == Parse::TK_STRING)
		{

			// Read key name.
			std::string keyName = (const char*)tk.GetToken(offset)->string;
			
			++offset;
			
			// Check for '='.
			if (tk.GetToken(offset)->token != TK_EQUALS)
				return false;
		
			++offset;
			
			// Check for EOF.
			if (tk.GetToken(offset)->token == Parse::TK_ZERO)
				return false;

			// Check if token can be read.
			std::string keyValue;

			if (
				tk.GetToken(offset)->token == Parse::TK_INTEGER ||
				tk.GetToken(offset)->token == Parse::TK_FLOAT ||
				tk.GetToken(offset)->token == Parse::TK_STRING)
			{
				keyValue += tk.GetToken(offset)->string;
			}
			else
			{
				return false;
			}
				
			++offset;
			
			Key_Set(keyName, keyValue);
		}
		else if (tk.GetToken(offset)->token == TK_SECTION_IDENTIFIER_L)
		{
			bool ret = ParseRecord(false, tk, offset);
			if (!ret)
				return false;
		}
		else
		{
			return false;
		}
	}

	if (tk.GetToken(offset)->token == Parse::TK_ZERO && !root)
		return false;
	
	++offset;
	
	if (recordName != "root")
		End();
	
	return true;
}

void Ar::GotoTop()
{
	m_currRecord = m_root;
	m_currKey = 0;
}

bool Ar::Merge(ArRecord* record)
{
	for (ArRecord::KeyItr i = record->m_keys.begin(); i != record->m_keys.end(); ++i)
		Key_Set(i->second->m_name, i->second->m_value);

	for (ArRecord::RecordItr i = record->m_records.begin(); i != record->m_records.end(); ++i)
	{
		Begin(i->second->m_id.m_name, i->second->m_id.m_index);

		if (!Merge(i->second))
			return false;

		End();
	}

	return true;
}

void Ar::Key_Begin(const std::string& name)
{
	Assert(m_currRecord);

	ArRecord::KeyItr i = m_currRecord->FindKey(name);

	if (i == m_currRecord->m_keys.end())
	{
		ArKey* key = m_currRecord->AddKey(name);

		m_currKey = key;
	}
	else
	{
		m_currKey = i->second;
	}
}

bool Ar::Key_IsSet(const std::string& key) const
{
	Assert(m_currRecord);
	
	Ar* _this = const_cast<Ar*>(this);

	ArRecord::KeyItr i = _this->m_currRecord->FindKey(key);

	return i == _this->m_currRecord->m_keys.end() ? false : i->second->m_assigned;
}

void Ar::Key_Set(const std::string& key, const std::string& value)
{
	Key_Begin(key);
	
	m_currKey->m_value = value;
	m_currKey->m_assigned = true;
}

const std::string& Ar::Key_Get(const std::string& key, const std::string& _default) const
{
	if (!Key_IsSet(key))
	{
		//Debug::VerboseMessage("Attempt to read non existing key value (%s).", key.c_str());
		return _default;
	}
	
	Ar* _this = const_cast<Ar*>(this);

	_this->Key_Begin(key);

	return m_currKey->m_value;
}

/*
void Ar::SetErrorString(const std::string& description) const
{
	Ar* _this = const_cast<Ar*>(this);

	_this->m_logger.Error(description.c_str());

}*/

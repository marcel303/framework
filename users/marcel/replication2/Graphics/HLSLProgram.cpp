#include "HLSLProgram.h"
#include "FileStream.h"
#include "StreamReader.h"

bool HLSLProgram::Load(const std::string& filename)
{
	try
	{
		FileStream stream;
		stream.Open(filename.c_str(), OpenMode_Read);

		StreamReader reader(&stream, false);

		if (filename.back() == 'g')
		{
			m_text.resize(stream.Length_get());

			reader.ReadBytes(&m_text[0], m_text.length());
		}
		else
		{
			int inputCount;

			reader.ReadBytes(&inputCount, 4);
		
			for (int i = 0; i < inputCount; ++i)
			{
				int nameLength;
				std::string name;
				int type;
				int registerIndex;

				reader.ReadBytes(&nameLength, 4);
				name.resize(nameLength);
				reader.ReadBytes(&name[0], nameLength);
				reader.ReadBytes(&type, 4);
				reader.ReadBytes(&registerIndex, 4);

				if (type == 0)
					continue;

				REGISTER_TYPE registerType;

				if (type == 1)
					registerType = SHREG_CONSTANT;
				else if (type == 2)
					registerType = SHREG_SAMPLER;
				else
					return false;

				p[name].Setup(registerType, registerIndex);
			}

			int textLength;
			reader.ReadBytes(&textLength, 4);
			m_text.resize(textLength);
			reader.ReadBytes(&m_text[0], textLength);
		}

		return true;
	}
	catch (std::exception&)
	{
		return false;
	}
}

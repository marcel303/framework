#include "File.h"
#include "FileSysMgr.h"
#include "HLSLProgram.h"

bool HLSLProgram::Load(const std::string& filename)
{
	ShFile f;

	if (!FileSysMgr::I().OpenFile(filename, FILE_READ, f))
		return false;

	if (filename[filename.length() - 1] == 'g')
	{
		m_text = f->Contents();
	}
	else
	{
		int inputCount;

		f->Read(&inputCount, 4);
		
		for (int i = 0; i < inputCount; ++i)
		{
			int nameLength;
			std::string name;
			int type;
			int registerIndex;

			f->Read(&nameLength, 4);
			name.resize(nameLength);
			f->Read(&name[0], nameLength);
			f->Read(&type, 4);
			f->Read(&registerIndex, 4);

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
		f->Read(&textLength, 4);
		m_text.resize(textLength);
		f->Read(&m_text[0], textLength);
	}

	return true;
}

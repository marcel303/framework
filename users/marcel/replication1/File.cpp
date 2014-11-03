#include <io.h>
#include "File.h"

File::File()
{
	m_file = 0;
}

File::~File()
{
	if (Opened())
		Close();
}

bool File::Open(const std::string& filename, FILE_OPENMODE mode)
{
	if (Opened())
		Close();

	m_filename = filename;

	char* modeStr = 0;

	if (mode == FILE_READ)
		modeStr = "rb";
	if (mode == FILE_READ_TEXT)
		modeStr = "rt";
	if (mode == FILE_WRITE)
		modeStr = "wb";
	if (mode == FILE_WRITE_TEXT)
		modeStr = "wt";

	m_file = fopen(filename.c_str(), modeStr);

	if (!m_file)
		return false;

	return true;
}

bool File::Close()
{
	if (m_file)
	{
		fclose(m_file);
		m_file = 0;
	}

	return true;
}

bool File::Opened()
{
	return m_file != 0;
}

std::string File::Contents()
{
	if (!Opened())
		return "";

	// TODO: Check result.
	fseek(m_file, 0, SEEK_END);
	int size = ftell(m_file);
	fseek(m_file, 0, SEEK_SET);

	// Read the entire file
	std::string r;
	r.resize(size);

	// TODO: Check result.
	Read(&r[0], size);

	return r;
}

bool File::Read(void* dst, int size)
{
	return fread(dst, size, 1, m_file) == 1;
}

bool File::Write(const void* src, int size)
{
	return fwrite(src, size, 1, m_file) == 1;
}

bool File::Write(const std::string& v)
{
	return Write(v.c_str(), v.length());
}

bool File::Seek(int position, FILE_SEEKMODE in_mode)
{
	int mode = 0;

	if (in_mode == SEEKMODE_ABS)
		mode = SEEK_SET;	
	if (in_mode == SEEKMODE_REL)
		mode = SEEK_CUR;
	
	return fseek(m_file, position, mode) == 0;
}

int File::GetLength()
{
	return _filelength(_fileno(m_file));
}

int File::GetPosition()
{
	return ftell(m_file);
}

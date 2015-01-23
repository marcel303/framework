#include "Stream.h"

class SubStream : public Stream
{
public:
	SubStream(Stream* stream, bool own, int position, int length);
	~SubStream();

	virtual void Open(const char* fileName, OpenMode mode);
	virtual void Close();
	virtual void Write(const void* bytes, int byteCount);
	virtual int Read(void* bytes, int byteCount);
	virtual int Length_get();
	virtual int Position_get();
	virtual void Seek(int seek, SeekMode mode);
	virtual bool EOF_get();

private:
	Stream* m_Stream;
	bool m_Own;
	int m_Position;
	int m_Length;
};

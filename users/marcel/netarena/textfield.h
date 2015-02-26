#pragma once

class TextField
{
public:
	TextField(int x, int y, int sx, int sy);

	bool tick(float dt);
	void draw();

	void open(int maxLength, bool canCancel);
	void close();
	bool isActive() const;

	const char * getText() const;

private:
	void complete();
	void addChar(char c);
	void removeChar();

	static const int kMaxBufferSize = 256;

	bool m_isActive;
	bool m_canCancel;

	int m_x;
	int m_y;
	int m_sx;
	int m_sy;

	int m_maxBufferSize;

	char m_buffer[kMaxBufferSize + 1];
	int m_bufferSize;

	int m_caretPosition;
	float m_caretTimer;
};

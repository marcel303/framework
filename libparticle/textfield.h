#pragma once

class EditorTextField
{
public:
	EditorTextField();

	bool tick(const float dt);
	void draw(const int ax, const int ay, const int asy, const int frontSize);

	void open(const int maxLength, const bool canCancel, const bool clearText);
	void close();
	bool isActive() const;

	void setText(const char * text);
	const char * getText() const;
	
	void setTextIsSelected(const bool isSelected);

private:
	void complete();
	void addChar(const char c);
	void removeChar();

	static const int kMaxBufferSize = 1024;

	bool m_isActive;
	bool m_canCancel;

	int m_maxBufferSize;

	char m_buffer[kMaxBufferSize + 1];
	int m_bufferSize;

	int m_caretPosition;
	float m_caretTimer;
	
	bool m_textIsSelected;
};

#pragma once

#include <stdarg.h>
#include <string>
#include <string.h>

#include <vector> // todo : remove. use LineReader to read individual lines

class LineWriter
{
	static const int kMaxFormattedTextSize = 1 << 12;
	static const int kDefaultBlockSize = 1 << 16;
	
	struct Block
	{
		Block * prev;
		
		char * text;
		size_t size;
		size_t capacity;
	};
	
	void addBlock();
	void newBlock(const size_t capacity);
	
	Block * blocks_tail = nullptr;
	
	char * text = nullptr;
	size_t text_size = 0;
	size_t text_capacity = 0;
	
public:
	~LineWriter();
	
	void Append(const char c)
	{
		if (text_size == text_capacity)
		{
			addBlock();
			newBlock(kDefaultBlockSize);
		}
		
		text[text_size] = c;
		
		text_size += 1;
	}
	
	void Append(const char * __restrict in_text)
	{
		size_t in_text_size = 0;
		
		while (in_text[in_text_size] != 0)
			in_text_size++;
		
		const size_t size = in_text_size;
		
		if (text_size + size <= text_capacity)
		{
			// space is available
		}
		else
		{
			const size_t block_size = size > kDefaultBlockSize ? size : kDefaultBlockSize;
			
			addBlock();
			newBlock(block_size);
		}
		
		char * __restrict text_ptr = text + text_size;
		
		for (size_t i = 0; i < in_text_size; ++i)
			text_ptr[i] = in_text[i];
			
		text_size += size;
	}
	
	void AppendIndentedLine(const int indentation, const char * __restrict in_text)
	{
		// compute space requirement
		
		size_t in_text_size = 0;
		
		while (in_text[in_text_size] != 0)
			in_text_size++;
		
		const size_t size = indentation + in_text_size + 1; // +1 for line break
		
		// ensure space is available
		
		if (text_size + size <= text_capacity)
		{
			// space is available
		}
		else
		{
			const size_t block_size = size > kDefaultBlockSize ? size : kDefaultBlockSize;
			
			addBlock();
			newBlock(block_size);
		}
		
		// copy data
		
		char * __restrict text_ptr = text + text_size;
		
		for (size_t i = 0; i < indentation; ++i)
			text_ptr[i] = '\t';
		
		text_ptr += indentation;
		
		for (size_t i = 0; i < in_text_size; ++i)
			text_ptr[i] = in_text[i];
	
		text_ptr[in_text_size] = '\n';
	
		text_size += size;
	}
	
	void AppendFormat(const char * format, ...);
	
	std::vector<std::string> ToLines(); // todo : remove
};

#include "lineWriter.h"
#include "StringEx.h"

void LineWriter::add_block()
{
	if (text_size == 0)
		return;
	
	Block * block = new Block();
	
	block->prev = blocks_tail;
	block->text = text;
	block->size = text_size;
	block->capacity = text_capacity;
	
	blocks_tail = block;
	
	text = nullptr;
	text_size = 0;
	text_capacity = 0;
}

void LineWriter::new_block(const size_t capacity)
{
	text = new char[capacity];
	text_size = 0;
	text_capacity = capacity;
}

LineWriter::~LineWriter()
{
	Block * block = blocks_tail;
	
	while (block != nullptr)
	{
		Block * prev = block->prev;
		
		//
		
		delete [] block->text;
		block->text = nullptr;
		
		delete block;
		block = nullptr;
		
		//
		
		block = prev;
	}
}

void LineWriter::append_format(const char * format, ...)
{
	va_list va;
	va_start(va, format);
	char text[kMaxFormattedTextSize];
	vsprintf_s(text, sizeof(text), format, va);
	va_end(va);

	append(text);
}

std::string LineWriter::to_string()
{
	std::string result;
	
	// commit current text
	
	if (text_size > 0)
	{
		add_block();
	}
	
	// early out when empty
	
	if (blocks_tail == nullptr)
	{
		return result;
	}
	
	// compute concatenation of all text blocks
	
	int num_blocks = 0;
	
	for (Block * block = blocks_tail; block != nullptr; block = block->prev)
		num_blocks++;
	
	std::vector<Block*> blocks;
	blocks.reserve(num_blocks);
	
	size_t total_size = 0;
	
	for (Block * block = blocks_tail; block != nullptr; block = block->prev)
	{
		total_size += block->size;
		
		blocks.push_back(block);
	}
	
	result.reserve(total_size);

	for (auto block_itr = blocks.rbegin(); block_itr != blocks.rend(); ++block_itr)
	{
		auto * block = *block_itr;
		
		result.insert(result.end(), block->text, block->text + block->size);
	}
	
	return result;
}

std::vector<std::string> LineWriter::to_lines() // todo : remove
{
	std::vector<std::string> result;
	
	const std::string total_text = to_string();

	size_t begin = 0;
	size_t end = 0;
	
	const char * str = total_text.c_str();
	
	size_t num_lines = 0;
	
	for (size_t i = 0; str[i] != 0; ++i)
		if (str[i] == '\n')
			num_lines++;
	
	result.reserve(num_lines);
	
	while (str[begin] != 0)
	{
		while (str[end] != 0 && str[end] != '\n')
			end++;
		
		result.emplace_back(std::string(str + begin, end - begin));
		
		begin = end + 1;
		end = begin;
	}
	
	return result;
}

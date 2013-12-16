#pragma once

#include <string>

class ImageId
{
public:
	ImageId();
	ImageId(const char* name);
	
	void Reset();
	bool IsSet_get() const;
	
	bool operator==(const ImageId& other) const;
	
	std::string mName;
};

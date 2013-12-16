#include "Debugging.h"
#include "ImageId.h"
#include "StringEx.h"

ImageId::ImageId()
{
}

ImageId::ImageId(const char* name)
{
	Assert(name != 0);
	Assert(strlen(name) > 0);
	
	mName = name;
}

void ImageId::Reset()
{
	mName = String::Empty;
}

bool ImageId::IsSet_get() const
{
	return mName != String::Empty;
}

bool ImageId::operator==(const ImageId& other) const
{
	if (!IsSet_get())
		return false;
	
	return mName == other.mName;
}

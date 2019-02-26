#include "component.h"
#include <string.h>

ComponentBase::~ComponentBase()
{
	if (id[0] != 0)
	{
		delete [] id;
	}
}

void ComponentBase::setId(const char * in_id)
{
	// free the existing id, if it isn't the empty string

	if (id[0] != 0)
		delete [] id;

	// if the new id is an empty string, avoid the new operation

	if (in_id[0] == 0)
	{
		id = "";
	}
	else
	{
		// otherwise allocate some memory and copy the string

		const size_t length = strlen(in_id);

		id = new char[length + 1];
		memcpy((char*)id, in_id, length + 1);
	}
}

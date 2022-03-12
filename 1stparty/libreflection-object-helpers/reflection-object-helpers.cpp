/*
	Copyright (C) 2020 Marcel Smit
	marcel303@gmail.com
	https://www.facebook.com/marcel.smit981

	Permission is hereby granted, free of charge, to any person
	obtaining a copy of this software and associated documentation
	files (the "Software"), to deal in the Software without
	restriction, including without limitation the rights to use,
	copy, modify, merge, publish, distribute, sublicense, and/or
	sell copies of the Software, and to permit persons to whom the
	Software is furnished to do so, subject to the following
	conditions:

	The above copyright notice and this permission notice shall be
	included in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
	OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
	NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
	HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
	WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
	OTHER DEALINGS IN THE SOFTWARE.
*/

#include "reflection-object-helpers.h"

#include "reflection-textio.h"

#include "lineReader.h"
#include "lineWriter.h"

bool copyObject(const TypeDB & typeDB, const Type * type, const void * srcObject, void * dstObject)
{
	LineWriter line_writer;
	if (object_tolines_recursive(typeDB, type, srcObject, line_writer, 0) == false)
		return false;
		
	auto lines = line_writer.to_lines();
	
	LineReader line_reader(lines, 0, 0); // todo : should pass lines as a pointer, as it doesn't copy the lines
	if (object_fromlines_recursive(typeDB, type, dstObject, line_reader) == false)
		return false;
	
	return true;
}

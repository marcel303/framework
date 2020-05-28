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

// todo : replace this with a decent sample set cache object and make it a global
// todo : move binaural sample set cache to somewhere else

namespace binaural
{
	struct HRIRSampleSet;
}

enum HRIRSampleSetType
{
	kHRIRSampleSetType_Cipic,
	kHRIRSampleSetType_Ircam,
	kHRIRSampleSetType_Mit,
	kHRIRSampleSetType_Oalsoft
};

void fillHrirSampleSetCache(const char * path, const char * name, const HRIRSampleSetType type);
void clearHrirSampleSetCache();
const binaural::HRIRSampleSet * getHrirSampleSet(const char * name);

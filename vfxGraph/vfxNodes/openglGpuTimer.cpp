/*
Copyright (C) 2017 Marcel Smit
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

#include "framework.h"
#include "openglGpuTimer.h"

const int OpenglGpuTimer::kMaxHistory;

static int s_numRunningTimers = 0;

OpenglGpuTimer::OpenglGpuTimer()
	: nextQuery(0)
	, nextRead(0)
	, numBusy(0)
	, elapsed(0)
{
	glGenQueries(kMaxHistory, queries);
	checkErrorGL();
}

OpenglGpuTimer::~OpenglGpuTimer()
{
	glDeleteQueries(kMaxHistory, queries);
	checkErrorGL();
}

void OpenglGpuTimer::begin()
{
	fassert(s_numRunningTimers == 0);
	s_numRunningTimers++;
	
	while (numBusy == kMaxHistory)
	{
		logDebug("waiting for GPU result!");
		
		poll();
	}
	
	glBeginQuery(GL_TIME_ELAPSED, queries[nextQuery]);
	checkErrorGL();
}

void OpenglGpuTimer::end()
{
	fassert(s_numRunningTimers == 1);
	s_numRunningTimers--;
	
	glEndQuery(GL_TIME_ELAPSED);
	checkErrorGL();
	
	numBusy++;
	
	if (nextQuery + 1 == kMaxHistory)
		nextQuery = 0;
	else
		nextQuery = nextQuery + 1;
}

void OpenglGpuTimer::poll()
{
	while (numBusy > 0)
	{
		GLuint query = queries[nextRead];
		
		GLint done = 0;
		
		glGetQueryObjectiv(query, GL_QUERY_RESULT_AVAILABLE, &done);
		checkErrorGL();
			
		if (done)
		{
			GLuint64 queryResult = 0;
		
			glGetQueryObjectui64v(query, GL_QUERY_RESULT, &queryResult);
			checkErrorGL();
			
			elapsed = queryResult;
		
			//logDebug("vfxGraph draw GPU time: %.2f", elapsed / 1000000.0);
			
			if (nextRead + 1 == kMaxHistory)
				nextRead = 0;
			else
				nextRead = nextRead + 1;
			
			numBusy--;
		}
		else
		{
			break;
		}
	}
}

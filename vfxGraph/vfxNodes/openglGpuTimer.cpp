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

#include <GL/glew.h> // glGenQueries
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
#if ENABLE_OPENGL
	glGenQueries(kMaxHistory, queries);
	checkErrorGL();
#endif
}

OpenglGpuTimer::~OpenglGpuTimer()
{
#if ENABLE_OPENGL
	glDeleteQueries(kMaxHistory, queries);
	checkErrorGL();
#endif
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
	
#if ENABLE_OPENGL
	glBeginQuery(GL_TIME_ELAPSED, queries[nextQuery]);
	checkErrorGL();
#endif
}

void OpenglGpuTimer::end()
{
	fassert(s_numRunningTimers == 1);
	s_numRunningTimers--;
	
#if ENABLE_OPENGL
	glEndQuery(GL_TIME_ELAPSED);
	checkErrorGL();
#endif
	
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
    #if ENABLE_OPENGL
		GLuint query = queries[nextRead];
		
		GLint done = 0;
		
		glGetQueryObjectiv(query, GL_QUERY_RESULT_AVAILABLE, &done);
		checkErrorGL();
    #else
        bool done = true;
    #endif
        
		if (done)
		{
        #if ENABLE_OPENGL
			GLuint64 queryResult = 0;
		
			glGetQueryObjectui64v(query, GL_QUERY_RESULT, &queryResult);
			checkErrorGL();
			
			elapsed = queryResult;
        #else
            elapsed = 0;
        #endif
		
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

//

const int OpenglGpuTimer2::kMaxHistory;

OpenglGpuTimer2::OpenglGpuTimer2()
	: nextQuery(0)
	, nextRead(0)
	, numBusy(0)
	, elapsed(0)
{
#if ENABLE_OPENGL
	glGenQueries(kMaxHistory * 2, (GLuint*)queries);
	checkErrorGL();
#endif
}

OpenglGpuTimer2::~OpenglGpuTimer2()
{
#if ENABLE_OPENGL
	glDeleteQueries(kMaxHistory * 2, (GLuint*)queries);
	checkErrorGL();
#endif
}

void OpenglGpuTimer2::begin()
{
	while (numBusy == kMaxHistory)
	{
		logDebug("waiting for GPU result!");
		
		poll();
	}
	
#if ENABLE_OPENGL
	glQueryCounter(queries[nextQuery][0], GL_TIMESTAMP);
	checkErrorGL();
#endif
}

void OpenglGpuTimer2::end()
{
#if ENABLE_OPENGL
	glQueryCounter(queries[nextQuery][1], GL_TIMESTAMP);
	checkErrorGL();
#endif
	
	numBusy++;
	
	if (nextQuery + 1 == kMaxHistory)
		nextQuery = 0;
	else
		nextQuery = nextQuery + 1;
}

void OpenglGpuTimer2::poll()
{
	while (numBusy > 0)
	{
    #if ENABLE_OPENGL
		GLuint query1 = queries[nextRead][0];
		GLuint query2 = queries[nextRead][1];
		
		GLint done = 0;
		
		glGetQueryObjectiv(query2, GL_QUERY_RESULT_AVAILABLE, &done);
		checkErrorGL();
    #else
        bool done = true;
    #endif
        
		if (done)
		{
        #if ENABLE_OPENGL
			GLuint64 t1 = 0;
			GLuint64 t2 = 0;
		
			glGetQueryObjectui64v(query1, GL_QUERY_RESULT, &t1);
			checkErrorGL();
			glGetQueryObjectui64v(query2, GL_QUERY_RESULT, &t2);
			checkErrorGL();
            
			elapsed = t2 - t1;
        #else
            elapsed = 0;
        #endif
		
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


/*
	oscpack -- Open Sound Control (OSC) packet manipulation library
	http://www.rossbencina.com/code/oscpack

	Copyright (c) 2004-2013 Ross Bencina <rossb@audiomulch.com>

	Permission is hereby granted, free of charge, to any person obtaining
	a copy of this software and associated documentation files
	(the "Software"), to deal in the Software without restriction,
	including without limitation the rights to use, copy, modify, merge,
	publish, distribute, sublicense, and/or sell copies of the Software,
	and to permit persons to whom the Software is furnished to do so,
	subject to the following conditions:

	The above copyright notice and this permission notice shall be
	included in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
	MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
	IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
	ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
	CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
	WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

/*
	The text above constitutes the entire oscpack license; however, 
	the oscpack developer(s) also make the following non-binding requests:

	Any person wishing to distribute modifications to the Software is
	requested to send the modifications to the original developer so that
	they can be incorporated into the canonical version. It is also 
	requested that these non-binding requests be included whenever the
	above license is reproduced.
*/
#include "ip/NetworkingUtils.h"

#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <cstring>



NetworkInitializer::NetworkInitializer() {}

NetworkInitializer::~NetworkInitializer() {}


unsigned long GetHostByName( const char *name )
{
    unsigned long result = 0;

    struct hostent *h = gethostbyname( name );
    if( h ){
        struct in_addr a;
        void * a_ptr = (void*)&a;
        void * h_ptr = (void*)h->h_addr_list[0];
        memcpy( a_ptr, h_ptr, h->h_length );
        result = ntohl(a.s_addr);
    }

    return result;
}

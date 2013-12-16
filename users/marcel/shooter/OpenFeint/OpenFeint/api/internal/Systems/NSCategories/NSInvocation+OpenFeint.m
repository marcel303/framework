////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// 
///  Copyright 2009 Aurora Feint, Inc.
/// 
///  Licensed under the Apache License, Version 2.0 (the "License");
///  you may not use this file except in compliance with the License.
///  You may obtain a copy of the License at
///  
///  	http://www.apache.org/licenses/LICENSE-2.0
///  	
///  Unless required by applicable law or agreed to in writing, software
///  distributed under the License is distributed on an "AS IS" BASIS,
///  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
///  See the License for the specific language governing permissions and
///  limitations under the License.
/// 
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#import "NSInvocation+OpenFeint.h"
#import <Foundation/Foundation.h>
#import <stdarg.h>

@implementation NSInvocation (OpenFeint)

+ (NSInvocation*)invocationWithTarget:(id)_target andSelector:(SEL)_selector
{
	NSMethodSignature* methodSig = [_target methodSignatureForSelector:_selector];
	NSInvocation* invocation = [NSInvocation invocationWithMethodSignature:methodSig];
	
	[invocation setTarget:_target];
	[invocation setSelector:_selector];
	
	return invocation;
}

// Remember to take the address of each argument rather than passing it as a literal!
+ (NSInvocation*)invocationWithTarget:(id)_target andSelector:(SEL)_selector andArguments:(void*)_addressOfFirstArgument, ...
{
	NSMethodSignature* methodSig = [_target methodSignatureForSelector:_selector];
	NSInvocation* invocation = [NSInvocation invocationWithMethodSignature:methodSig];
	
	[invocation setTarget:_target];
	[invocation setSelector:_selector];

	unsigned int numArgs = [methodSig numberOfArguments];

	if (2 < numArgs)
	{
		va_list varargs;
		va_start(varargs, _addressOfFirstArgument);
		[invocation setArgument:_addressOfFirstArgument atIndex:2];

		for (int argIdx=3; argIdx<numArgs; ++argIdx)
		{
			// We could do type-checking here someday
			void* argp = va_arg(varargs, void *);
			[invocation setArgument:argp atIndex:argIdx];
		}
		
		va_end(varargs);
	}
	
	return invocation;
}


@end

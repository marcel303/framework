////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// 
///  Copyright 2010 Aurora Feint, Inc.
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

#import <UIKit/UIWindow.h>
#import "IPhoneOSIntrospection.h"

#import "UIWindow+OpenFeint.h"


@implementation UIWindow (OpenFeint)

+ (CGSize)OFgetKeyboardSize:(NSDictionary* )dict;
{
#if defined(__IPHONE_3_2)
	if (is3Point2SystemVersion())
	{
		return [[dict objectForKey:UIKeyboardFrameEndUserInfoKey] CGRectValue].size;
	}
	else
	{
		//To get no warnings and be backwards compatable we need to actually use the string for
		//UIKeyboardBoundsUserInfoKey.  Its silly-sauce, if you think of a different way to fix this
		//with not warnings please do.
		return [[dict objectForKey:@"UIKeyboardBoundsUserInfoKey"] CGRectValue].size;
	}
#else
	return [[dict objectForKey:UIKeyboardBoundsUserInfoKey] CGRectValue].size;
#endif
	
}

@end

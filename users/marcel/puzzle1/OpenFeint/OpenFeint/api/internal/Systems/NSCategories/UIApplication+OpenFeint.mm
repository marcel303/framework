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

#import <UIKit/UIApplication.h>
#import "IPhoneOSIntrospection.h"

#import "UIApplication+OpenFeint.h"


@implementation UIApplication (OpenFeint)

- (void)OFhideStatusBar:(BOOL)hidden animated:(BOOL)animate
{
#if defined(__IPHONE_3_2)
	if (is3Point2SystemVersion())
	{
		[(id)self setStatusBarHidden:hidden withAnimation:(animate ? UIStatusBarAnimationFade : UIStatusBarAnimationNone)];
	}
	else
	{
		[(id)self setStatusBarHidden:hidden animated:animate];
	}
#else
	[self setStatusBarHidden:hidden animated:animate];
#endif
}

@end

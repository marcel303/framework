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

#if !defined(__IPHONE_3_2)

#import <Foundation/Foundation.h>

// Due to a bug in the XCode 3.2.2 Beta interface builder when compiling against SDKs < 3.2, we need
// this proxy class.  This should be removed once Apple releases a patch that fixes the following issue:

// "Building an iPhone application that uses UITableViewCell objects encoded in XIB files and then running
// that application in the simulator or on a device with a version less than 3.2 will result in a runtime
// exception. To workaround this, use the simulator with the 3.2 SDK."

// http://developer.apple.com/iphone/prerelease/library/releasenotes/General/RN-iPhoneSDK-3_2/index.html
// or
// http://developer.apple.com/iphone/library/releasenotes/General/RN-iPhoneSDK-3_2/index.html

@interface UITableViewCellContentView : NSObject
{
}

+ (id)alloc;
+ (id)allocWithZone:(NSZone *)zone;

@end

#endif

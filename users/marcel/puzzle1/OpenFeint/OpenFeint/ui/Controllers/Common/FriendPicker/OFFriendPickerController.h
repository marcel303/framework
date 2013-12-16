//  Copyright 2009-2010 Aurora Feint, Inc.
// 
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//  
//  	http://www.apache.org/licenses/LICENSE-2.0
//  	
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.

#pragma once

#import "OFFramedNavigationControllerBehavior.h"
#import "OFTableSequenceControllerHelper.h"
#import "OFCallbackable.h"

@class OFUser;

@protocol OFFriendPickerDelegate<NSObject>

@required
- (void)pickerFinishedWithSelectedUser:(OFUser*)selectedUser;

@optional
- (void)pickerCancelled;

@end

@interface OFFriendPickerController : OFTableSequenceControllerHelper<OFCallbackable, OFFramedNavigationControllerBehavior>

{
	id<OFFriendPickerDelegate> delegate;
	NSString* promptText;
	NSString* scopedByApplicationId;
    
#ifdef __IPHONE_3_2
    UIPopoverController *owningPopoverController;
#endif
}

@property (nonatomic, retain) id<OFFriendPickerDelegate> delegate;
@property (nonatomic, retain) NSString* promptText;
@property (nonatomic, retain) NSString* scopedByApplicationId;

#ifdef __IPHONE_3_2
@property (nonatomic, assign) UIPopoverController *owningPopoverController;
#endif

+ (void)launchPickerWithDelegate:(id<OFFriendPickerDelegate>)_delegate;
+ (void)launchPickerWithDelegate:(id<OFFriendPickerDelegate>)_delegate promptText:(NSString*)_promptText;
+ (void)launchPickerWithDelegate:(id<OFFriendPickerDelegate>)_delegate promptText:(NSString*)_promptText mustHaveApplicationId:(NSString*)_applicationId;

@end

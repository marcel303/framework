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

#import <UIKit/UIKit.h>

@protocol OFKeyboardAdjustment
@required
- (CGFloat)_keyboardAdjustment;
@end

@interface OFRootController : UIViewController
{
@package
	UIView* containerView;
    UIView* bgOverlay;
    UIImageView *bgDropShadow;
    
	UIViewController* contentController;
    UIViewController* nonFullscreenModalController;
	
	NSString* transitionIn;
	NSString* transitionOut;
	
	BOOL keyboardShowing;
}

@property (nonatomic, retain) IBOutlet UIView *containerView;
@property (nonatomic, retain) IBOutlet UIView* bgOverlay;
@property (nonatomic, retain) UIImageView* bgDropShadow;

@property (nonatomic, readonly) UIViewController* contentController;
@property (nonatomic, retain) UIViewController* nonFullscreenModalController;

@property (nonatomic, retain) NSString* transitionIn;
@property (nonatomic, retain) NSString* transitionOut;

- (void)showController:(UIViewController*)_controller;
- (void)hideController;

- (void)cleanupSubviews;

@end

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

#import <UIKit/UIKit.h>
#import "OFNavigationController.h"
#import "OFContentFrameView.h"
#import "OFViewController.h"

@interface OFIntroNavigationController : OFViewController {
    OFContentFrameView *contentFrameView;
    OFNavigationController *navController;
    
    BOOL fullscreenFrame;
	
	UIView* loadingView;
}

@property (nonatomic, retain) OFContentFrameView *contentFrameView;
@property (nonatomic, retain) OFNavigationController *navController;
@property (nonatomic, assign) BOOL fullscreenFrame;

+ (OFIntroNavigationController*)activeIntroNavigationController;

- (id)initWithNavigationController:(OFNavigationController*)aNavController;
- (CGFloat)bottomPadding;
- (void)setFullscreenFrame:(BOOL)fullscreen animated:(BOOL)animated;

- (void)displayLoadingView:(UIView*)_loadingView;
- (void)removeLoadingView;

@end

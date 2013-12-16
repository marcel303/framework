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

#import "OFDependencies.h"
#import "OFRootController.h"
#import "OpenFeint+Private.h"
#import "OpenFeint+UserOptions.h"
#import "OpenFeint+NSNotification.h"
#import "OFImageLoader.h"
#import "OFTabbedDashboardController.h"

#import <QuartzCore/QuartzCore.h>

@implementation OFRootController

@synthesize containerView, bgOverlay, bgDropShadow;
@synthesize contentController, nonFullscreenModalController;
@synthesize transitionIn, transitionOut;

- (void)viewDidLoad
{
    if ([OpenFeint isLargeScreen])
    {
        self.containerView.backgroundColor = [UIColor clearColor];
        
        self.bgDropShadow = [[[UIImageView alloc] initWithFrame:containerView.bounds] autorelease];
        bgDropShadow.image = [[OFImageLoader loadImage:@"OFDashboardShadow.png"] stretchableImageWithLeftCapWidth:30.f topCapHeight:30.f];
		CGRect dashboardFrame = [OpenFeint getDashboardBounds];
		bgDropShadow.frame = CGRectInset(dashboardFrame, -(dashboardFrame.origin.x), -(dashboardFrame.origin.y));
        [containerView insertSubview:bgDropShadow atIndex:0];
    }
}

- (void)viewWillAppear:(BOOL)animated
{
	[super viewWillAppear:animated];
	
	if ([OpenFeint isLargeScreen])
	{
		[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(_keyboardWillShow:) name:UIKeyboardWillShowNotification object:nil];
		[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(_keyboardWillHide:) name:UIKeyboardWillHideNotification object:nil];
		[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(_dashboardWillRotate:) name:OFNSNotificationDashboardOrientationChanged object:nil];
	}
}

- (void)viewWillDisappear:(BOOL)animated
{
	[super viewWillDisappear:animated];

	if ([OpenFeint isLargeScreen])
	{
		[[NSNotificationCenter defaultCenter] removeObserver:self name:UIKeyboardWillShowNotification object:nil];
		[[NSNotificationCenter defaultCenter] removeObserver:self name:UIKeyboardWillHideNotification object:nil];
		[[NSNotificationCenter defaultCenter] removeObserver:self name:OFNSNotificationDashboardOrientationChanged object:nil];
	}
}

- (void)_adjustController:(UIViewController*)controller toCompensateForKeyboard:(BOOL)shouldAdjust
{
	if ([controller isKindOfClass:[OFTabBarController class]])
	{
		OFTabBarController* tabController = (OFTabBarController*)controller;
		for (OFTabBarItem* item in tabController.tabBarItems)
		{
			[self _adjustController:item.viewController toCompensateForKeyboard:shouldAdjust];
		}
	}
	else if ([controller isKindOfClass:[OFFramedNavigationController class]])
	{
		OFFramedNavigationController* navController = (OFFramedNavigationController*)controller;

		CGFloat heightToUse = 0.f;
		if ([navController.topViewController conformsToProtocol:@protocol(OFKeyboardAdjustment)])
		{
			heightToUse = [(id<OFKeyboardAdjustment>)navController.topViewController _keyboardAdjustment];
		}

		[navController adjustForKeyboard:shouldAdjust && (heightToUse > 0.f) ofHeight:heightToUse];
	}
}

- (void)_adjustForKeyboard:(UIInterfaceOrientation)orientation
{
	BOOL landscape = UIInterfaceOrientationIsLandscape(orientation);
	
	if (keyboardShowing && landscape)
	{
		CGPoint center = containerView.center;
		center.y = CGRectGetMidY(containerView.bounds) + 30.f;
		containerView.center = center;
	}
	else if (landscape)
	{
		CGPoint center = containerView.center;
		center.y = 373.f;
		containerView.center = center;
	}
	else
	{
		CGPoint center = containerView.center;
		center.y = 491.f;
		containerView.center = center;
	}

	[self _adjustController:contentController toCompensateForKeyboard:keyboardShowing && landscape];
}

- (void)_keyboardWillShow:(NSNotification*)notification
{
	if (keyboardShowing)
		return;
		
	keyboardShowing = YES;

	[self _adjustForKeyboard:[OpenFeint getDashboardOrientation]];
}

- (void)_keyboardWillHide:(NSNotification*)notification
{
	if (!keyboardShowing)
		return;
		
	keyboardShowing = NO;

	[self _adjustForKeyboard:[OpenFeint getDashboardOrientation]];
}

- (void)_dashboardWillRotate:(NSNotification*)notification
{
	if (!keyboardShowing)
		return;
		
	NSNumber* newOri = (NSNumber*)[[notification userInfo] objectForKey:OFNSNotificationInfoNewOrientation];
	NSNumber* oldOri = (NSNumber*)[[notification userInfo] objectForKey:OFNSNotificationInfoOldOrientation];
	
	if (newOri && oldOri)
	{
		UIInterfaceOrientation newOrientation = (UIInterfaceOrientation)[newOri intValue];
		UIInterfaceOrientation oldOrientation = (UIInterfaceOrientation)[oldOri intValue];
		
		// changing from portrait to landscape or vice versa
		if (UIInterfaceOrientationIsLandscape(newOrientation) != UIInterfaceOrientationIsLandscape(oldOrientation))
		{
			[self _adjustForKeyboard:newOrientation];
		}
	}
}

- (void)animationDidStop:(CAAnimation*)_animation finished:(BOOL)_finished
{
	if (contentController.view.hidden)
	{
		[contentController viewDidDisappear:YES];
		[contentController.view removeFromSuperview];
		contentController.view = nil;
		OFSafeRelease(contentController);
	}
	else
	{
		[contentController viewDidAppear:YES];
	}
	[OpenFeint animationDidStop:_animation finished:_finished];
    
    if ([OpenFeint initialDashboardModalContentURL] && [contentController isKindOfClass:[OFTabbedDashboardController class]])
    {
        [contentController performSelector:@selector(_presentModalContentController) withObject:nil afterDelay:0.5f];
        [OpenFeint setInitialDashboardModalContentURL:nil];
    }
}

- (void)showController:(UIViewController*)_controller
{
    OFSafeRelease(contentController);
	contentController = [_controller retain];
	[contentController viewWillAppear:YES];
    
    contentController.view.frame = [OpenFeint getDashboardBounds];    
    
	[containerView addSubview:contentController.view];
    
    //to keep it from appearing underneath the background fader
    [containerView.superview bringSubviewToFront:containerView];
    
	CATransition* animation = [CATransition animation];
	animation.type = kCATransitionMoveIn;
	animation.subtype = transitionIn;
	animation.duration = 0.5f;
	animation.timingFunction = [CAMediaTimingFunction functionWithName:kCAMediaTimingFunctionEaseInEaseOut];
	animation.delegate = self;
	[[containerView layer] addAnimation:animation forKey:nil];
    
    bgOverlay.alpha = 0.f;
    [UIView beginAnimations:nil context:nil];
    [UIView setAnimationDuration:0.5f];
    bgOverlay.alpha = 1.f;
    [UIView commitAnimations];
}

- (void)hideController
{
	if (contentController == nil)
		return;
		
	[contentController viewWillDisappear:YES];

	CATransition* animation = [CATransition animation];
	animation.type = kCATransitionReveal;
	animation.subtype = transitionOut;
	animation.duration = 0.5f;
	animation.timingFunction = [CAMediaTimingFunction functionWithName:kCAMediaTimingFunctionEaseInEaseOut];
	animation.delegate = self;
	[[containerView layer] addAnimation:animation forKey:nil];
    
	containerView.hidden = YES;
    
    [UIView beginAnimations:nil context:nil];
    [UIView setAnimationDuration:0.5f];
    bgOverlay.alpha = 0.f;
    [UIView commitAnimations];    
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
	return interfaceOrientation == [OpenFeint getDashboardOrientation];
}

- (void)presentModalViewController:(UIViewController *)_modalViewController animated:(BOOL)animated
{
    if ([OpenFeint isLargeScreen])
    {
        self.nonFullscreenModalController = _modalViewController;
        [self.contentController.view addSubview:nonFullscreenModalController.view];
        nonFullscreenModalController.view.frame = self.contentController.view.bounds;;
        
        if (animated)
        {
            CATransition* animation = [CATransition animation];
            animation.type = kCATransitionMoveIn;
            animation.subtype = kCATransitionFromTop;
            animation.duration = 0.5f;
            animation.timingFunction = [CAMediaTimingFunction functionWithName:kCAMediaTimingFunctionEaseInEaseOut];
            [[nonFullscreenModalController.view layer] addAnimation:animation forKey:nil];
        }
    }
    else
    {
        [super presentModalViewController:_modalViewController animated:animated];
    }
}

- (void)dismissModalViewControllerAnimated:(BOOL)animated
{
    if ([OpenFeint isLargeScreen])
    {
        nonFullscreenModalController.view.hidden = YES;
        
        if (animated)
        {
            CATransition* animation = [CATransition animation];
            animation.type = kCATransitionReveal;
            animation.subtype = kCATransitionFromBottom;
            animation.duration = 0.5f;
            animation.timingFunction = [CAMediaTimingFunction functionWithName:kCAMediaTimingFunctionEaseInEaseOut];
            [[nonFullscreenModalController.view layer] addAnimation:animation forKey:nil];
            
            nonFullscreenModalController.view.hidden = YES;
        }
    }
    else
    {
        [super dismissModalViewControllerAnimated:animated];
    } 
}

- (UIViewController*)modalViewController
{
	if (nonFullscreenModalController != nil)
		return nonFullscreenModalController;
		
	return [super modalViewController];
}

- (void)cleanupSubviews {
    while([[self.view subviews] count] > 0)
    {
        UIView *aSubview = [[self.view subviews] objectAtIndex:0];
        [aSubview removeFromSuperview];
    }
}

- (void)dealloc
{
	OFSafeRelease(contentController);
    OFSafeRelease(nonFullscreenModalController);
	OFSafeRelease(containerView);
    OFSafeRelease(bgOverlay);
    OFSafeRelease(bgDropShadow);
    
	self.transitionIn = nil;
	self.transitionOut = nil;

	[super dealloc];
}

@end

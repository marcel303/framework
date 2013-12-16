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

#import "OFIntroNavigationController.h"
#import "OpenFeint+Private.h"

static OFIntroNavigationController *sActiveIntroNavigationController = nil;

@implementation OFIntroNavigationController

@synthesize contentFrameView, navController, fullscreenFrame;

+ (OFIntroNavigationController*)activeIntroNavigationController
{
    return sActiveIntroNavigationController;
}

- (id)initWithNavigationController:(OFNavigationController*)aNavController
{
    self = [self init];
    if (self != nil)
    {
        self.navController = aNavController;
        self.view; // force view to load
        
        sActiveIntroNavigationController = self;
    }
    return self;
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    self.view.clipsToBounds = YES;
    //self.view.backgroundColor = [UIColor colorWithWhite:0.93f alpha:1.0];
    
    navController.view.frame = CGRectInset(self.view.bounds, 6, 6);
    navController.loadingViewParent = self.view;
    
    [navController viewWillAppear:NO];
    [self.view addSubview:navController.view];
    [navController viewDidAppear:NO];
    
    self.contentFrameView = [[[OFContentFrameView alloc] initWithFrame:self.view.bounds] autorelease];
    [self.view addSubview:contentFrameView];
    
    // Forces layout of content frame
    self.fullscreenFrame = fullscreenFrame;
    
    if (![OpenFeint isLargeScreen]) {
        self.view.backgroundColor = [UIColor colorWithWhite:0.93f alpha:1.0];
    }
}

- (void)viewWillAppear:(BOOL)animated
{
    [super viewWillAppear:animated];
    if (![OpenFeint isLargeScreen])
    {
        contentFrameView.frame = self.view.bounds;
    }
}

-(void)viewDidAppear:(BOOL)animated 
{
    //FogBug #1414 ok, this requires a bit of explanation
    //basically, for some reason internal to the Apple API, this value is getting reset to a bogus value
    //if I set it earlier, then it gets totally messed up
    //this corrects it at the time of showing, which means it does make a slight jump at the end of the animation, but that's as good as I can seem to get
    if ([OpenFeint isLargeScreen])
    {
        self.navController.view.frame = CGRectMake(6,6,628,468);
    }
    [super viewDidAppear:animated];
}

- (void)dealloc
{
    self.contentFrameView = nil;
    self.navController = nil;
    sActiveIntroNavigationController = nil;
	OFSafeRelease(loadingView);
    [super dealloc];
}

- (void)setFullscreenFrame:(BOOL)fullscreen
{
    [self setFullscreenFrame:fullscreen animated:YES];
}

- (void)setFullscreenFrame:(BOOL)fullscreen animated:(BOOL)animated
{
    fullscreenFrame = fullscreen;
    
    CGFloat bottomHeight = [self bottomPadding];
    CGRect contentFrame = [OpenFeint getDashboardBounds];
    contentFrame.size.height -= bottomHeight;
	contentFrame.origin = CGPointZero;
    
	CGRect loadingFrame = contentFrame;
	loadingFrame.size.height -= [OpenFeint isLargeScreen] ? 10.f : 0.f;
    
    if (animated)
    {
        [UIView beginAnimations:nil context:nil];
        [UIView setAnimationBeginsFromCurrentState:YES];
    }
    
    contentFrameView.frame = contentFrame;
	loadingView.frame = loadingFrame;
    
    if (animated)
    {
        [UIView commitAnimations];
    }
}

- (CGFloat)bottomPadding
{
    if (fullscreenFrame) return 0.f;
    
    if ([OpenFeint isLargeScreen])
    {
        return 55.f;
    }
    else if ([OpenFeint isInLandscapeMode])
    {
        return 40.f;
    }
    else
    {
        return 50.f;
    }    
}

- (void)displayLoadingView:(UIView*)_loadingView
{
	[self removeLoadingView];
	loadingView = [_loadingView retain];
	
	loadingView.clipsToBounds = YES;
	
	CGRect frame = contentFrameView.frame;
	frame.size.height -= [OpenFeint isLargeScreen] ? 10.f : 0.f;
	[loadingView setFrame:frame];
	[self.view insertSubview:loadingView belowSubview:self.contentFrameView];
}

- (void)removeLoadingView
{
	OFSafeRelease(loadingView);
}

@end


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

#import "OFWebNavController.h"
#import "OFControllerLoader.h"
#import "OFViewHelper.h"

#import "OpenFeint+Private.h"
#import "OFProvider.h"
#import "MPURLRequestParameter.h"
#import "OFContentFrameView.h"
#import "OFImageView.h"
#import "IPhoneOSIntrospection.h"



@implementation OFWebNavController

@synthesize url, actionMap, dismissOnPopToRoot;
@synthesize webView, transitionImage;

#pragma mark Init

+ (OFWebNavController*)controllerWithTitle:(NSString*)title url:(NSURL*)url useContentFrame:(BOOL)useContentFrame
{
    return [[[OFWebNavController alloc] initWithTitle:title url:url useContentFrame:useContentFrame] autorelease];
}

- (id)initWithTitle:(NSString*)aTitle url:(NSURL*)aUrl useContentFrame:(BOOL)_useContentFrame
{
    self = [self initWithNibName:nil bundle:nil];
    if (self)
    {
        self.title = aTitle;
        self.url = aUrl;
        self.actionMap = [NSMutableDictionary dictionary];
        self.dismissOnPopToRoot = NO;
        
        // useContentFrame = _useContentFrame
        useContentFrame = NO;  // Always inset as long as we are inheriting form OFFramedNavigationController -Alex
        
        // Setup action map
        [self mapAction:@"startLoading"         toSelector:@selector(actionStartLoading:)];
        [self mapAction:@"domLoaded"            toSelector:@selector(actionDomLoaded:)];
        [self mapAction:@"navPop"               toSelector:@selector(actionNavPop)];
        [self mapAction:@"alert"                toSelector:@selector(actionAlert:)];
        [self mapAction:@"addBarButton"         toSelector:@selector(actionAddBarButton:)];
        [self mapAction:@"dismissNavController" toSelector:@selector(actionDismissNavController)];
    }
    return self;
}

- (void)dealloc
{
    [self.webView stopLoading];
	self.webView.delegate = nil;
	self.webView = nil;
    self.transitionImage = nil;
    
    self.url = nil;
    self.actionMap = nil;
    [super dealloc];
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    self.view.clipsToBounds = YES;
    self.view.backgroundColor = [UIColor colorWithWhite:0.8 alpha:1.0];
    self.view.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    [self performSelector:@selector(_fixNavBar) withObject:nil afterDelay:0.05]; // FIXME: not sure why this needs to be fixed...
    
    CGRect contentFrame = self.view.frame;
    if ([OpenFeint isInLandscapeMode]) contentFrame.size = CGSizeMake(contentFrame.size.height, contentFrame.size.width);
    self.view.frame = contentFrame;
    
    contentFrame.size.height -= self.navigationBar.frame.size.height;
    
    if (useContentFrame)
    {
        CGRect frameFrame = self.view.bounds;
        frameFrame.origin.y    += self.navigationBar.frame.size.height;
        frameFrame.size.height -= self.navigationBar.frame.size.height;
        OFContentFrameView *framedView = [[[OFContentFrameView alloc] initWithFrame:frameFrame] autorelease];
        framedView.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
        [self.view addSubview:framedView];
        contentFrame = CGRectInset(contentFrame, 6, 6);        
    }
    
    self.transitionImage = [[[UIImageView alloc] initWithFrame:contentFrame] autorelease];
    transitionImage.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    transitionImage.contentMode = UIViewContentModeTopLeft;
    
    self.webView = [[[UIWebView alloc] initWithFrame:contentFrame] autorelease];
    webView.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    webView.delegate = self;
    webView.alpha = 0.f;
    
#ifdef __IPHONE_3_0
    if      ([webView respondsToSelector:@selector(setDataDetectorTypes:)]) webView.dataDetectorTypes = UIDataDetectorTypeNone; // 3.x
    else if ([webView respondsToSelector:@selector(detectsPhoneNumbers)])  [webView performSelector:@selector(setDetectsPhoneNumbers:) withObject:[NSNumber numberWithBool:NO]]; // 2.x
#else
    if ([webView respondsToSelector:@selector(detectsPhoneNumbers)])  [webView performSelector:@selector(setDetectsPhoneNumbers:) withObject:[NSNumber numberWithBool:NO]]; // 2.x
#endif
    
    // Load the initial request into the webview
    NSArray *params = [NSArray arrayWithObjects:
                       [[[MPURLRequestParameter alloc] initWithName:@"landscape"   andValue:[NSString stringWithFormat:@"%u", [OpenFeint isInLandscapeMode]]] autorelease],
                       [[[MPURLRequestParameter alloc] initWithName:@"orientation" andValue:[NSString stringWithFormat:@"%u", [OpenFeint getDashboardOrientation]]] autorelease],
                       nil];
    
    NSURLRequest* request = [[[OpenFeint provider] 
                              getRequestForAction:[url path]
                              withParameters:params
                              withHttpMethod:@"GET"
                              withSuccess:OFDelegate()
                              withFailure:OFDelegate()
                              withRequestType:OFActionRequestForeground
                              withNotice:nil
                              requiringAuthentication:true] getConfiguredRequest];
    
    [webView loadRequest:request];
    [self showLoadingIndicator];
    
    OFWebNavContentController *controller = [OFWebNavContentController controllerWithWebView:webView imageView:transitionImage isInset:useContentFrame];
    [self pushViewController:controller animated:NO];
}

- (void)_fixNavBar {
    self.navigationBar.frame = CGRectMake(0, 0, self.navigationBar.frame.size.width, self.navigationBar.frame.size.height);
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
	return interfaceOrientation == [OpenFeint getDashboardOrientation];
}

#pragma mark UIWebViewDelegate methods

- (void)webView:(UIWebView *)webView didFailLoadWithError:(NSError *)error
{
    [[[[UIAlertView alloc] initWithTitle:@"Failed to load"
                                 message:@"Sorry, but we had a problem displaying this screen.  Please try again soon."
                                delegate:nil
                       cancelButtonTitle:@"OK"
                       otherButtonTitles:nil] autorelease] show];
    NSLog(@"OFWebNavController load error: %@", error);
    
    [self hideLoadingIndicator];
    if ([self.viewControllers count] <= 1 && self.dismissOnPopToRoot)
    {
        [self actionDismissNavController];
    }
}

// If url to load is an action in the form of "openfeint://action/foobar?bar=foo",
// process it.  Otherise just load up the page as normal.
- (BOOL)webView:(UIWebView *)_webView shouldStartLoadWithRequest:(NSURLRequest *)request navigationType:(UIWebViewNavigationType)navigationType
{
    NSURL *requestURL = [request URL];
    if ([[requestURL scheme] isEqualToString:@"openfeint"])
    {
        if ([[requestURL host] isEqualToString:@"action"])
        {
            [self performAction:requestURL];
        }
        else if ([[requestURL host] isEqualToString:@"controller"])
        {
            [self pushControllerURL:requestURL];
        }
        return NO;
    }
    else if ([[requestURL host] isEqualToString:@"phobos.apple.com"] || [[requestURL host] isEqualToString:@"click.linksynergy.com"])
	{
		[[UIApplication sharedApplication] openURL:requestURL];
		return NO;
	}    
    else
    {
        return YES;
    }
}

- (void)webViewDidFinishLoad:(UIWebView *)webView
{
//    [self actionDomLoaded:nil];
}

#pragma mark UINavigationController

- (void)pushViewController:(UIViewController *)viewController animated:(BOOL)animated
{
    if (useContentFrame && ![viewController isKindOfClass:[OFWebNavContentController class]])
    {
        UIScrollView* topScrollView = OFViewHelper::findFirstScrollView(viewController.view);
        topScrollView.contentInset = UIEdgeInsetsMake(6, 6, 6, 6);
    }
    
    [super pushViewController:viewController animated:YES];
}

- (UIViewController*)popViewControllerAnimated:(BOOL)animated
{
    OFWebNavContentController *topController = (OFWebNavContentController*)self.topViewController;
    OFWebNavContentController *backController = (OFWebNavContentController*)[self.viewControllers objectAtIndex:[self.viewControllers count]-2];
    
    if ([topController isKindOfClass:[OFWebNavContentController class]])
    {
        [topController freeze];
    }
    
    if ([backController isKindOfClass:[OFWebNavContentController class]])
    {
        [backController thaw];
        
        // Only tell the webview to go back if we are also coming from a web view
        if ([topController isKindOfClass:[OFWebNavContentController class]])
        {
            webView.alpha = 0.f;
            [webView stringByEvaluatingJavaScriptFromString:@"OF.goBack()"];
        }
    }
    
    return [super popViewControllerAnimated:animated];
}

- (NSArray*)popToRootViewControllerAnimated:(BOOL)animated
{
    if (dismissOnPopToRoot)
    {
        [self actionDismissNavController];
        return nil;
    }
    else
    {
        return [super popToRootViewControllerAnimated:YES];
    }
}

#pragma mark OFNavigationController


- (OFFramedNavigationControllerVisibilityFlags)_visibilityFlagsForController:(UIViewController*)viewController
{
    OFFramedNavigationControllerVisibilityFlags v = [(id)super _visibilityFlagsForController:viewController];
    v.showNavBar = YES;
    return v;
}

#pragma mark -
#pragma mark Native Action Handling

#pragma mark - Action utility methods

// Breaks a url encoded URL like:"openfeint://action/actionName?foo=bar&zing=bang"
// into a dictionary like: { "foo":"bar", "zing":"bang" }
- (NSDictionary*)optionsForAction:(NSURL*)actionURL
{
    NSString *query = [actionURL query];
    if (query) {
        query = [query stringByReplacingOccurrencesOfString:@"\%20" withString:@" "];
        NSArray *pairs = [query componentsSeparatedByString:@"&"];
        NSMutableDictionary *dict = [NSMutableDictionary dictionary];
        
        for (NSString *pair in pairs) {
            NSArray *pairArray = [pair componentsSeparatedByString:@"="];
            [dict setObject:[pairArray objectAtIndex:1] forKey:[pairArray objectAtIndex:0]];
        }
        
        return dict;        
    }
    else
    {
        return nil;
    }
}

// Map a action name string to specific method to handle it in native code
// Selector can optionally accept a single argument, an NSDictionary of options
- (void)mapAction:(NSString*)actionName toSelector:(SEL)selector
{
    actionName = [@"/" stringByAppendingString:actionName];
    NSInvocation *invocation = [NSInvocation invocationWithMethodSignature:[self methodSignatureForSelector:selector]];
    [invocation setSelector:selector];
    [invocation setTarget:self];
    [actionMap setObject:invocation forKey:actionName];
}

// Dispatcher for mapping a string action name and it's arguments to action handler methods
- (void)performAction:(NSURL*)actionURL
{
    NSString *name = [actionURL path];
    NSDictionary *options = [self optionsForAction:actionURL];
    
    NSLog(@"Performing action: %@", actionURL);
    
    NSInvocation *invocation = [actionMap objectForKey:name];
    if (invocation)
    {
        if ([[invocation methodSignature] numberOfArguments] > 2) // Always has at least 2 args. 3 means method takes one argument.
        {
            [invocation setArgument:&options atIndex:2];
        }
        [invocation invokeWithTarget:self];
    }
    else
    {
        NSLog(@"Unhandled action! %@", actionURL);
    }
}

#pragma mark - Special Action Handlers

// Tell the webview that we tapped the right nav bar button.
- (void)didTapBarButton
{
    [webView stringByEvaluatingJavaScriptFromString:@"OF.tappedBarButton()"];
    
    // Toggle the bar button between Edit/Done
    UIBarButtonItem *barButton = self.topViewController.navigationItem.rightBarButtonItem;
    if ([barButton.title isEqualToString:@"Edit"])
    {
        barButton.title = @"Done";
        barButton.style = UIBarButtonItemStyleDone;
    }
    else if ([barButton.title isEqualToString:@"Done"])
    {
        barButton.title = @"Edit";
        barButton.style = UIBarButtonItemStyleBordered;
    }
}

// Push a webview at the url
- (void)pushURLString:(NSString*)contentURLString
{
    NSURL *contentURL = [NSURL URLWithString:contentURLString relativeToURL:[[webView request] URL]];
    NSString *js = [NSString stringWithFormat:@"OF.navigateToUrl('%@', {})", [contentURL absoluteString]];
    [webView stringByEvaluatingJavaScriptFromString:js];
}

// Create and push a native view controller from a url
- (void)pushControllerURL:(NSURL*)controllerURL
{
    NSString *controllerName = [[controllerURL path] stringByReplacingOccurrencesOfString:@"/" withString:@""];
    [self pushControllerName:controllerName options:[self optionsForAction:controllerURL]];
}

// Create and push a native view controller
- (void)pushControllerName:(NSString*)controllerName options:(NSDictionary*)options
{
    UIViewController *controller = OFControllerLoader::loadWithParamsDictionary(controllerName, options, nil);
    
    NSString *globalBarButtonName = [webView stringByEvaluatingJavaScriptFromString:@"OF.flags.globalBarButton"];
    if (!controller.navigationItem.rightBarButtonItem && ![globalBarButtonName isEqualToString:@""])
    {
        controller.navigationItem.rightBarButtonItem = self.topViewController.navigationItem.rightBarButtonItem;
    }
    
    [self pushViewController:controller animated:YES];
}

#pragma mark - Specific Action Handlers

- (bool)canReceiveCallbacksNow
{
    return YES;
}

// Same as tapping the back button on the nav bar, but can be called form the view.
- (void)actionNavPop
{
    [self popViewControllerAnimated:YES];
}

// Prepare to load a new page by performing a "forward" transition and showing the loading view.
- (void)actionStartLoading:(NSDictionary*)options
{
    OFWebNavContentController *backController = (OFWebNavContentController*)self.topViewController;
    if ([backController isKindOfClass:[OFWebNavContentController class]]) {
        [self showLoadingIndicator];
        [backController freeze];
        
        webView.alpha = 0.f;        
    }
    
    OFWebNavContentController *newController = [OFWebNavContentController controllerWithWebView:webView imageView:transitionImage isInset:useContentFrame];
    newController.title = [options objectForKey:@"title"];
    [self pushViewController:newController animated:YES];    
}

// Called after the content and all <img> tags load.  Show the view.
- (void)actionDomLoaded:(NSDictionary*)options
{
    [self hideLoadingIndicator];
    
    // Fade in
    webView.alpha = 0.0;
    [UIView beginAnimations:@"fadeIn" context:nil];
    self.webView.alpha = 1.0;
    [UIView commitAnimations];
    
    // Optionally add a bar button
    if (!self.topViewController.navigationItem.rightBarButtonItem && ([options objectForKey:@"barButton"] || [options objectForKey:@"barButtonImage"]))
    {
        [self actionAddBarButton:options];
    }
    
    // Set page title
    NSString *pageTitle = [options objectForKey:@"title"];
    if (!self.topViewController.title && pageTitle && ![pageTitle isEqualToString:@""])
    {
        self.topViewController.title = pageTitle;
    }
    
    // Optionally set the page title image
    NSString *titleImage = [options objectForKey:@"titleImage"];
    if (!self.topViewController.navigationItem.titleView && titleImage && ![titleImage isEqualToString:@"null"])
    {
        NSURL *titleImageURL = [NSURL URLWithString:titleImage relativeToURL:[[webView request] URL]];
        
        OFImageView *titleImageView = [[[OFImageView alloc] init] autorelease];
        titleImageView.frame = CGRectMake(0, 0, self.navigationBar.frame.size.width/2, self.navigationBar.frame.size.height);
        titleImageView.opaque = NO;
        titleImageView.useSharpCorners = YES;
        [titleImageView setImageUrl:[titleImageURL absoluteString] crossFading:YES];
        self.topViewController.navigationItem.titleView = titleImageView;
    }
    
    if (is2PointOhSystemVersion())
    {
        OFWebNavContentController *c = (OFWebNavContentController*)self.topViewController;
        if ([c isKindOfClass:[OFWebNavContentController class]])
        {
            [c freeze];
            [c thaw];
        }
    }
}

// Display an alert view with a given title and message
- (void)actionAlert:(NSDictionary*)options
{
    [[[[UIAlertView alloc] initWithTitle:[options objectForKey:@"title"]
                                 message:[options objectForKey:@"message"]
                                delegate:nil
                       cancelButtonTitle:@"OK"
                       otherButtonTitles:nil] autorelease] show];
}

// Adds a bar button to the right side of the navbar.
- (void)actionAddBarButton:(NSDictionary*)options
{
    UIBarButtonItem *navBarButton = nil;
    
    if ([options objectForKey:@"barButtonImage"])
    {
        NSData *imageData = [NSData dataWithContentsOfURL:[NSURL URLWithString:[options objectForKey:@"barButtonImage"]]];
        UIImage *image = [UIImage imageWithData:imageData];
        
        navBarButton = [[[UIBarButtonItem alloc] initWithImage:image
                                                         style:UIBarButtonItemStylePlain
                                                        target:self
                                                        action:@selector(didTapBarButton)] autorelease];
    }
    else if ([options objectForKey:@"barButton"])
    {
        navBarButton = [[[UIBarButtonItem alloc] initWithTitle:[options objectForKey:@"barButton"]
                                                         style:UIBarButtonItemStyleBordered
                                                        target:self
                                                        action:@selector(didTapBarButton)] autorelease];
    }
    
    [self.topViewController.navigationItem performSelector:@selector(setRightBarButtonItem:)
                                                withObject:navBarButton
                                                afterDelay:0.1];
}

// Dismisses this OFWebNavController as if it was a modal controller
- (void)actionDismissNavController
{
    [[OpenFeint getRootController] dismissModalViewControllerAnimated:YES];
}

@end

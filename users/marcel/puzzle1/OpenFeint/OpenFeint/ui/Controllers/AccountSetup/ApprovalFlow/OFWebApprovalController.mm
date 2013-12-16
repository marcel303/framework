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

#import "OFWebApprovalController.h"
#import "OpenFeint+Private.h"
#import "OpenFeint+Settings.h"
#import "OFControllerLoader.h"
#import "OFUserFeintApprovalController.h"
#import "OFIntroNavigationController.h"


@implementation OFWebApprovalController

- (void)loadWebContent
{
    [super loadWebContent];
    
    // Prep views
    self.view.backgroundColor = [UIColor grayColor];
    mWebView.backgroundColor = [UIColor grayColor];
    mWebView.alpha = 0.0;
    
    // Put the webview in the main window view so it renders it's content
    CGRect webViewFrame = [OpenFeint getDashboardBounds];
    webViewFrame.origin = CGPointZero;
    mWebView.frame = webViewFrame;
    [[OpenFeint getTopApplicationWindow] addSubview:mWebView];
    [[OpenFeint getTopApplicationWindow] sendSubviewToBack:mWebView];
    
    // Set timeout for approval modal to appear
    loadingTimeoutTimer = [NSTimer scheduledTimerWithTimeInterval:5 target:self selector:@selector(show) userInfo:nil repeats:NO];
}

- (NSString*)getAction
{
    NSString *escapedAppName = [[OpenFeint applicationDisplayName] stringByAddingPercentEscapesUsingEncoding:NSASCIIStringEncoding];
    return [NSString stringWithFormat:@"/web_views/simple_intro/%@", escapedAppName];
}

- (NSString*)getTitle
{
    return @"Approve OF";
}

- (void)setApprovedDelegate:(const OFDelegate&)approvedDelegate andDeniedDelegate:(const OFDelegate&)deniedDelegate
{
	mApprovedDelegate = approvedDelegate;
	mDeniedDelegate = deniedDelegate;
}

- (NSDictionary*)getDispatchDictionary
{
    NSMutableDictionary *dict = [NSMutableDictionary dictionaryWithDictionary:[super getDispatchDictionary]];
    [dict setObject:@"_webActionApprove" forKey:@"approve"];
    [dict setObject:@"_webActionDecline" forKey:@"decline"];
    return dict;
}

- (void)_webActionApprove
{
    [OpenFeint userDidApproveFeint:YES accountSetupCompleteDelegate:mApprovedDelegate];
    [[OFIntroNavigationController activeIntroNavigationController] setFullscreenFrame:NO];
}

- (void)_webActionDecline
{
    [OpenFeint userDidApproveFeint:NO];
    [OpenFeint allowErrorScreens:YES];
	[OpenFeint dismissRootControllerOrItsModal];
    
	mDeniedDelegate.invoke();
}

// We have failed, for who knows what reason.  Simply swap out this fancy webview for a native one we know works great.
- (void)webView:(UIWebView *)webView didFailLoadWithError:(NSError *)error
{
    NSLog(@"Error Loading Webview: %@", error);
    
    OFUserFeintApprovalController *nativeController = (OFUserFeintApprovalController*)OFControllerLoader::load(@"UserFeintApproval");
    [nativeController setApprovedDelegate:mApprovedDelegate andDeniedDelegate:mDeniedDelegate];
    [[OFIntroNavigationController activeIntroNavigationController] setFullscreenFrame:NO animated:NO];
    
    [[self retain] autorelease]; // Ensure we exist when we pop ourselves
    UINavigationController *navController = self.navigationController; // get a pointer to the nav controller since we will lose it when we pop
    
    [navController popViewControllerAnimated:NO];
    [navController pushViewController:nativeController animated:NO];
    
    isLoaded = YES;
    [self show];
}

- (void)switchToNativeIfNotYetLoaded
{
    if (!isLoaded)
    {
        OFLog(@"OFWebApprovalController: LOAD TIMEOUT :: Switching to native view");
        [mWebView stopLoading];
    }
}

- (void)webViewDidFinishLoad:(UIWebView *)webView
{
    NSString *bodyClass = [webView stringByEvaluatingJavaScriptFromString:@"document.body.className"];
    if ([bodyClass isEqualToString:@"ipad"] ||
        [bodyClass isEqualToString:@"landscape"] ||
        [bodyClass isEqualToString:@"portrait"])
    {
        [self performSelector:@selector(webViewDidFinishDelayedLoad:) withObject:webView afterDelay:1.0];
    }
    else
    {
        [self webView:webView didFailLoadWithError:[NSError errorWithDomain:@"ServerError" code:0 userInfo:nil]];
    }
}

- (void)webViewDidFinishDelayedLoad:(UIWebView *)webView
{
    [super webViewDidFinishLoad:webView];
    isLoaded = YES;
    
    [UIView beginAnimations:nil context:nil];
    mWebView.alpha = 1.0;
    [UIView commitAnimations];
    
    [self show];
}

- (void)show
{
    if (!isShown)
    {
        [OpenFeint presentRootControllerWithModal:[OFIntroNavigationController activeIntroNavigationController]];
        isShown = YES;
        
        if (!isLoaded)
        {
            [NSTimer scheduledTimerWithTimeInterval:4 target:self selector:@selector(switchToNativeIfNotYetLoaded) userInfo:nil repeats:NO];
        }
    }
}

- (void)viewWillAppear:(BOOL)animated
{
    [super viewWillAppear:animated];
    mWebView.alpha = isLoaded ? 1.0 : 0.0;
    [self.view sizeToFit];
}

- (void)viewDidAppear:(BOOL)animated
{
    [super viewDidAppear:animated];
    mWebView.alpha = isLoaded ? 1.0 : 0.0;
}

- (void)viewDidDisappear:(BOOL)animated
{
    [super viewDidDisappear:animated];
}

- (void)dealloc
{
    [super dealloc];
}


@end

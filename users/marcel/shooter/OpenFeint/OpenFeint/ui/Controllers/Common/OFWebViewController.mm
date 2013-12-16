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

#import "OFWebViewController.h"
#import "OFWebViewController+Overridables.h"
#import "OFControllerLoader.h"
#import "OpenFeint+Private.h"
#import "OFProvider.h"
#import "MPURLRequestParameter.h"
#import "NSObject+WeakLinking.h"
#import "OFApplicationDescriptionController.h"

@implementation OFWebViewController

- (void)loadWebContent
{
    self.view;
    didFirstLoad = YES;
    
	bool landscape = [OpenFeint isInLandscapeMode];
	MPURLRequestParameter* landscapeParam = [[[MPURLRequestParameter alloc] initWithName:@"landscape" andValue:[NSString stringWithFormat:@"%u", landscape]] autorelease];
    
    NSString *orientationString;
    if ([OpenFeint isLargeScreen]) {
        orientationString = @"ipad";
    } else if ([OpenFeint isInLandscapeMode]) {
        orientationString = @"landscape";
    } else {
        orientationString = @"Portrait";
    }
    
	MPURLRequestParameter* orientationParam = [[[MPURLRequestParameter alloc] initWithName:@"orientation" andValue:orientationString] autorelease];
	NSMutableArray* params = [NSMutableArray arrayWithObject:landscapeParam];
	[params addObject:orientationParam];
	
	NSArray* overriddenParams = [self getParameters];
	if (overriddenParams)
	{
		[params addObjectsFromArray:overriddenParams];
	}
	
	NSString* action = [self getAction];
	NSURLRequest* request = 
	[[[OpenFeint provider] 
	  getRequestForAction:action
	  withParameters:params
	  withHttpMethod:@"GET"
	  withSuccess:OFDelegate()
	  withFailure:OFDelegate()
	  withRequestType:OFActionRequestForeground
	  withNotice:[OFNotificationData foreGroundDataWithText:[self notificationString]]
	  requiringAuthentication:true] getConfiguredRequest];
	
	[mWebView loadRequest:request];
}

- (void)viewWillAppear:(BOOL)animated
{
	[super viewWillAppear:animated];
	
	self.title = [self getTitle];
	
    // Ensure that all our views are properly sized.
    [self.view sizeToFit];
    [mWebView removeFromSuperview];
    [self.view addSubview:mWebView];
    mWebView.frame = self.view.bounds;
    mWebView.alpha = 1;
    [mWebView setNeedsLayout];
    
    // Recenter to loading indicator
    mLoadingView.frame = CGRectMake(round(self.view.frame.size.width/2  - mLoadingView.frame.size.width/2),
                                    round(self.view.frame.size.height/2 - mLoadingView.frame.size.height/2),
                                    mLoadingView.frame.size.width,
                                    mLoadingView.frame.size.height);
    
    if (![OpenFeint isLargeScreen])
    {
        NSString *orientation = [OpenFeint isInLandscapeMode] ? @"landscape" : @"portrait";
        [mWebView stringByEvaluatingJavaScriptFromString:[NSString stringWithFormat:@"setOrientation('%@')", orientation]];
    }
    
	[self.view bringSubviewToFront:mLoadingView];
    
    if (!didFirstLoad)
    {
        [self loadWebContent];
    }
}

- (UIWebView*)webView
{
    if (!mWebView) self.view;
    return mWebView;
}

- (void)viewWillDisappear:(BOOL)animated
{
	[super viewWillDisappear:animated];
}

- (void)loadView
{	
	UIView* contentView = [[UIView alloc] initWithFrame:CGRectZero];
	contentView.backgroundColor = [UIColor clearColor];
	
	self.view = contentView;
	self.view.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
	self.view.autoresizesSubviews = YES;
	
	[contentView release];

	[mWebView stopLoading];
	mWebView.delegate = nil;
	OFSafeRelease(mWebView);
	
	mWebView = [[UIWebView alloc] initWithFrame:CGRectZero];
	mWebView.delegate = self;
	mWebView.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
	mWebView.autoresizesSubviews = YES;	
	mWebView.backgroundColor = [UIColor clearColor];
	mWebView.scalesPageToFit = YES;
	mWebView.autoresizingMask = (UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight);
	
	[mWebView trySet:@"dataDetectorTypes" with:OF_OS_3_ENUM_ARG(UIDataDetectorTypeNone) elseSet:@"detectsPhoneNumbers" with:NO];
	
	[self.view addSubview:mWebView];
	
	CGRect fullscreen = [[UIScreen mainScreen] bounds];
	CGPoint indicatorCenter = CGPointZero;
	if ([OpenFeint isInLandscapeMode])
	{
		indicatorCenter = CGPointMake(fullscreen.size.height * 0.5f, fullscreen.size.width * 0.5f);
	}
	else
	{
		indicatorCenter = CGPointMake(fullscreen.size.width * 0.5f, fullscreen.size.height * 0.5f);
	}
	float const kLoadingViewSize = 40.0f;
	mLoadingView = [[UIActivityIndicatorView alloc] initWithActivityIndicatorStyle:UIActivityIndicatorViewStyleWhiteLarge];
	mLoadingView.frame = CGRectMake(indicatorCenter.x - (kLoadingViewSize * 0.5f), indicatorCenter.y - (kLoadingViewSize * 0.5f), kLoadingViewSize, kLoadingViewSize);
	mLoadingView.hidesWhenStopped = YES;
	[mLoadingView stopAnimating];
	[self.view addSubview:mLoadingView];
}

- (void)webViewDidStartLoad:(UIWebView *)webView
{
	[mLoadingView startAnimating];
}

- (void)webViewDidFinishLoad:(UIWebView *)webView
{
	[mLoadingView stopAnimating];
}

- (BOOL)_webActionPopController:(NSArray*)args
{
	[self.navigationController popViewControllerAnimated:YES];
	return YES;
}

- (BOOL)_webActionPushController:(NSArray*)args
{
	if (args.count == 1)
	{
		UIViewController* vc = OFControllerLoader::load((NSString*)[args objectAtIndex:0], nil);
		if (vc)
		{
			[self.navigationController pushViewController:vc animated:YES];
			return YES;
		}
	}
	return NO;
}

- (BOOL)_webActionPushIPurchase:(NSArray*)args
{
	if (args.count == 1)
	{
		OFApplicationDescriptionController* iPurchaseController = [OFApplicationDescriptionController applicationDescriptionForId:(NSString*)[args objectAtIndex:0] appBannerPlacement:@"dedicatedClient"];
		if (iPurchaseController)
		{
			[self.navigationController pushViewController:iPurchaseController animated:YES];
			return YES;
		}
	}
	return NO;
}

- (NSDictionary*) getDispatchDictionary
{
	return [NSDictionary dictionaryWithObjectsAndKeys:
			@"_webActionPopController:", @"pop",
			@"_webActionPushController:", @"push",
			@"_webActionPushIPurchase:", @"ipurchase",
			nil];
}

- (BOOL)webView:(UIWebView *)webView shouldStartLoadWithRequest:(NSURLRequest *)request navigationType:(UIWebViewNavigationType)navigationType 
{
	NSURL* url = [request URL];
	NSString* scheme = [url scheme];
	
	// Check for our specific magic urls first.
	if ([scheme isEqualToString:@"openfeint"])
	{
		// here comes the magic
		NSString* res = [url resourceSpecifier];
		NSArray* split = [res componentsSeparatedByString:@"?"];
		if (split.count == 2 || [res rangeOfString:@"?"].location == NSNotFound)
		{
			if (!mDispatch)
			{
				mDispatch = [[self getDispatchDictionary] retain];
			}
			
			NSString* target = [mDispatch objectForKey:[split objectAtIndex:0]];
			if (target)
			{
				SEL targetSel = NSSelectorFromString(target);
				if ([self respondsToSelector:targetSel])
				{
					NSArray* args = nil;
                    if (split.count > 1) args = [[split objectAtIndex:1] componentsSeparatedByString:@"&"];
					[self performSelector:targetSel withObject:args];
					return NO;
				}
			}
		}
	}
	else if ([[url host] isEqualToString:@"phobos.apple.com"])
	{
		[[UIApplication sharedApplication] openURL:url];
		return NO;
	}
	else if([[url host] isEqualToString:@"oauth_redirect.openfeint.com"])
	{
		NSString* action = [url path];
		NSURLRequest* request = 
		[[[OpenFeint provider] 
		  getRequestForAction:action
		  withParameters:nil
		  withHttpMethod:@"GET"
		  withSuccess:OFDelegate()
		  withFailure:OFDelegate()
		  withRequestType:OFActionRequestForeground
		  withNotice:[OFNotificationData foreGroundDataWithText:@"Downloaded"]
		  requiringAuthentication:true] getConfiguredRequest];
		
		[mWebView loadRequest:request];		
		return NO;
	}
	
	return YES;
}


- (void)webView:(UIWebView *)webView didFailLoadWithError:(NSError *)error
{
    //this happens when a new request is made before the last finishes, this is fine
    if (error.code == NSURLErrorCancelled || error.code == 102)
    {
        return;
    }

	[mLoadingView stopAnimating];
	
	NSString* centeredErrorMessage = 
	@"<html>"
	@"	<head>"
	@"		<meta http-equiv=\"Content-Type\" content=\"text/html; charset=iso-8859-1\">"
	@"		<meta name=\"viewport\" content=\"width=device-width, user-scalable=no\" />"
	@"	</head>"
	@"	<body bgcolor=\"#000000\" style=\"width:320px;height:480px;margin:0px;padding:0px; color: white; font-family: Helvetica\">"
	@"		<table width=\"320\" height=\"480\">"
	@"			<tr valign=\"middle\">"
	@"				<td align=\"center\">%@</td>"
	@"			</tr>"
	@"		</table>"
	@"	</body>"
	@"</html>";
	
	
    NSString* errorString = nil;
	
	if (error.code == NSURLErrorNotConnectedToInternet && error.domain == NSURLErrorDomain)
	{
		errorString = @"You must be connected to the Internet.<br /><br /><i style=\"color: gray\">Try again once you're online.</i>";
	}
	else
	{
		errorString = [NSString stringWithFormat:@"Oops! An Error Occurred. Press the Back button to return to the previous screen.<br /><br /><i style=\"color: gray\">%@</i>", error.localizedDescription];
	}										
	
	[mWebView loadHTMLString:[NSString stringWithFormat:centeredErrorMessage, errorString] baseURL:nil];
}

- (void)dealloc
{	
	[mWebView stopLoading];
	mWebView.delegate = nil;
	OFSafeRelease(mWebView);
	
	OFSafeRelease(mLoadingView);
	OFSafeRelease(mDispatch);
	[super dealloc];
}

@end

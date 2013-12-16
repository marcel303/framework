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

#import "OFWebNavContentController.h"


@implementation OFWebNavContentController

@synthesize webView, imageView;

+ (OFWebNavContentController*)controllerWithWebView:(UIWebView*)webView imageView:(UIImageView*)imageView isInset:(BOOL)isInset
{
    return [[[OFWebNavContentController alloc] initWithWebView:webView imageView:imageView isInset:isInset] autorelease];
}

- (id)initWithWebView:(UIWebView*)_webView imageView:(UIImageView*)_imageView isInset:(BOOL)_isInset
{
    self = [self initWithNibName:nil bundle:nil];
    if (self)
    {
        // isInset = _isInset;
        isInset = NO;  // Hard code this as long as we are inheriting form OFFramedNavigationController -Alex
        self.imageView = _imageView;
        self.webView = _webView;
        [self.view addSubview:webView];
    }
    return self;
}

- (void)viewDidLoad {
    [super viewDidLoad];
    
    imageView.frame = self.view.bounds;
    webView.frame = self.view.bounds;
    
    if (isInset) {
        imageView.frame = CGRectInset(imageView.frame, 6, 6);
        webView.frame = CGRectInset(webView.frame, 6, 6);
    }
    
    webView.backgroundColor = [UIColor clearColor];
    webView.opaque = NO;
    
    [self.view addSubview:webView];
}

- (void)freeze
{
    imageView.frame = webView.frame;
    UIGraphicsBeginImageContext(webView.bounds.size);
    [webView.layer renderInContext:UIGraphicsGetCurrentContext()];
    imageView.image = UIGraphicsGetImageFromCurrentImageContext();
    UIGraphicsEndImageContext();
    
    [self.view addSubview:imageView];
}

- (void)thaw
{
    CGRect webViewFrame = webView.frame;
    webViewFrame.origin = CGPointMake(6, 6);
    webView.frame = webViewFrame;
    
    [self.view addSubview:webView];
}

- (void)dealloc
{
    self.webView = nil;
    self.imageView = nil;
    [super dealloc];
}

@end

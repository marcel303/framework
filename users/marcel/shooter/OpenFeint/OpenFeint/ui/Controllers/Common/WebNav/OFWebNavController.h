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
#import <QuartzCore/QuartzCore.h>
#import "OFFramedNavigationController.h"
#import "OFWebNavContentController.h"

@interface OFWebNavController : OFFramedNavigationController <UIWebViewDelegate, OFCallbackable> {
    NSURL *url;
    NSMutableDictionary *actionMap;
    BOOL dismissOnPopToRoot;
    BOOL useContentFrame;
    
    UIWebView *webView;
    UIImageView *transitionImage;
}

@property (nonatomic, copy) NSURL *url;
@property (nonatomic, retain) NSMutableDictionary *actionMap;
@property (nonatomic, assign) BOOL dismissOnPopToRoot;

@property (nonatomic, retain) UIWebView *webView;
@property (nonatomic, retain) UIImageView *transitionImage;

+ (OFWebNavController*)controllerWithTitle:(NSString*)title url:(NSURL*)url useContentFrame:(BOOL)useContentFrame;
- (id)initWithTitle:(NSString*)aTitle url:(NSURL*)aUrl useContentFrame:(BOOL)useContentFrame;

- (void)didTapBarButton;
- (void)pushURLString:(NSString*)contentURLString;
- (void)pushControllerURL:(NSURL*)controllerURL;
- (void)pushControllerName:(NSString*)controllerName options:(NSDictionary*)options;

- (NSDictionary*)optionsForAction:(NSURL*)actionURL;
- (void)mapAction:(NSString*)actionName toSelector:(SEL)selector;
- (void)performAction:(NSURL*)actionURL;

- (void)actionNavPop;
- (void)actionStartLoading:(NSDictionary*)options;
- (void)actionDomLoaded:(NSDictionary*)options;
- (void)actionAlert:(NSDictionary*)options;
- (void)actionAddBarButton:(NSDictionary*)options;
- (void)actionDismissNavController;

@end

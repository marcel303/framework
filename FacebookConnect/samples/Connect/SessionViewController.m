/*
 * Copyright 2009 Facebook
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *    http://www.apache.org/licenses/LICENSE-2.0

 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

#import "SessionViewController.h"
#import "FBConnect/FBConnect.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
// This application will not work until you enter your Facebook application's API key here:

static NSString* kApiKey = @"<YOUR API KEY>";

// Enter either your API secret or a callback URL (as described in documentation):
static NSString* kApiSecret = nil; // @"<YOUR SECRET KEY>";
static NSString* kGetSessionProxy = nil; // @"<YOUR SESSION CALLBACK)>";

///////////////////////////////////////////////////////////////////////////////////////////////////

@implementation SessionViewController

@synthesize label = _label;

///////////////////////////////////////////////////////////////////////////////////////////////////
// NSObject

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil {
  if (self = [super initWithNibName:@"SessionViewController" bundle:nibBundleOrNil]) {
    if (kGetSessionProxy) {
      _session = [[FBSession sessionForApplication:kApiKey getSessionProxy:kGetSessionProxy
                             delegate:self] retain];
    } else {
      _session = [[FBSession sessionForApplication:kApiKey secret:kApiSecret delegate:self] retain];
    }
  }
  return self;
}

- (void)dealloc {
  [_session release];
  [super dealloc];
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// UIViewController

- (void)viewDidLoad {
  [_session resume];
  _loginButton.style = FBLoginButtonStyleWide;
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
  return YES;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// FBDialogDelegate

- (void)dialog:(FBDialog*)dialog didFailWithError:(NSError*)error {
  _label.text = [NSString stringWithFormat:@"Error(%d) %@", error.code,
    error.localizedDescription];
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// FBSessionDelegate

- (void)session:(FBSession*)session didLogin:(FBUID)uid {
  _label.text = @"";
  _permissionButton.hidden = NO;
  _feedButton.hidden       = NO;
  _statusButton.hidden     = NO;
  _photoButton.hidden      = NO;

  NSString* fql = [NSString stringWithFormat:
    @"select uid,name from user where uid == %lld", session.uid];

  NSDictionary* params = [NSDictionary dictionaryWithObject:fql forKey:@"query"];
  [[FBRequest requestWithDelegate:self] call:@"facebook.fql.query" params:params];
}

- (void)sessionDidNotLogin:(FBSession*)session {
  _label.text = @"Canceled login";
}

- (void)sessionDidLogout:(FBSession*)session {
  _label.text = @"Disconnected";
  _permissionButton.hidden = YES;
  _feedButton.hidden       = YES;
  _statusButton.hidden     = YES;
  _photoButton.hidden      = YES;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// FBRequestDelegate

- (void)request:(FBRequest*)request didLoad:(id)result {
  if ([request.method isEqualToString:@"facebook.fql.query"]) {
    NSArray* users = result;
    NSDictionary* user = [users objectAtIndex:0];
    NSString* name = [user objectForKey:@"name"];
    _label.text = [NSString stringWithFormat:@"Logged in as %@", name];
  } else if ([request.method isEqualToString:@"facebook.users.setStatus"]) {
    NSString* success = result;
    if ([success isEqualToString:@"1"]) {
      _label.text = [NSString stringWithFormat:@"Status successfully set"]; 
    } else {
      _label.text = [NSString stringWithFormat:@"Problem setting status"]; 
    }
  } else if ([request.method isEqualToString:@"facebook.photos.upload"]) {
    NSDictionary* photoInfo = result;
    NSString* pid = [photoInfo objectForKey:@"pid"];
    _label.text = [NSString stringWithFormat:@"Uploaded with pid %@", pid];
  }
}

- (void)request:(FBRequest*)request didFailWithError:(NSError*)error {
  _label.text = [NSString stringWithFormat:@"Error(%d) %@", error.code,
    error.localizedDescription];
}

///////////////////////////////////////////////////////////////////////////////////////////////////

- (void)askPermission:(id)target {
  FBPermissionDialog* dialog = [[[FBPermissionDialog alloc] init] autorelease];
  dialog.delegate = self;
  dialog.permission = @"status_update";
  [dialog show];
}

- (void)publishFeed:(id)target {
	FBStreamDialog* dialog = [[[FBStreamDialog alloc] init] autorelease];
	dialog.delegate = self;
	dialog.userMessagePrompt = @"Example prompt";
	dialog.attachment = @"{\"name\":\"Facebook Connect for iPhone\",\"href\":\"http://developers.facebook.com/connect.php?tab=iphone\",\"caption\":\"Caption\",\"description\":\"Description\",\"media\":[{\"type\":\"image\",\"src\":\"http://img40.yfrog.com/img40/5914/iphoneconnectbtn.jpg\",\"href\":\"http://developers.facebook.com/connect.php?tab=iphone/\"}],\"properties\":{\"another link\":{\"text\":\"Facebook home page\",\"href\":\"http://www.facebook.com\"}}}";
	// replace this with a friend's UID
	// dialog.targetId = @"999999";
	[dialog show];
}

- (void)setStatus:(id)target {
  NSString *statusString = @"Testing iPhone Connect SDK";
	NSDictionary *params = [NSDictionary dictionaryWithObjectsAndKeys:
							statusString, @"status",
							@"true", @"status_includes_verb",
							nil];
	[[FBRequest requestWithDelegate:self] call:@"facebook.users.setStatus" params:params];
}

- (void)uploadPhoto:(id)target {
  NSString *path = @"http://www.facebook.com/images/devsite/iphone_connect_btn.jpg";
  NSURL    *url  = [NSURL URLWithString:path];
  NSData   *data = [NSData dataWithContentsOfURL:url];
  UIImage  *img  = [[UIImage alloc] initWithData:data];
  
  NSDictionary *params = nil;
  [[FBRequest requestWithDelegate:self] call:@"facebook.photos.upload" params:params dataParam:(NSData*)img];
}

@end

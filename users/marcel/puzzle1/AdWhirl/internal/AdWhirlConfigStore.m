/*

 AdWhirlConfigStore.m

 Copyright 2010 Google Inc.

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.

 */

#import "AdWhirlConfigStore.h"
#import "AdWhirlLog.h"
#import "AWNetworkReachabilityWrapper.h"
#import "AdWhirlError.h"

static AdWhirlConfigStore *gStore = nil;

@interface AdWhirlConfigStore ()

- (void)fetchDataForConfig:(AdWhirlConfig *)config;

@end


@implementation AdWhirlConfigStore

+ (AdWhirlConfigStore *)sharedStore {
  if (gStore == nil) {
    gStore = [[AdWhirlConfigStore alloc] init];
  }
  return gStore;
}

- (id)init {
  self = [super init];
  if (self != nil) {
    configs_ = [[NSMutableDictionary alloc] init];
  }
  return self;
}

- (AdWhirlConfig *)getConfig:(NSString *)appKey
                    delegate:(id<AdWhirlConfigDelegate>)delegate {
  AdWhirlConfig *config = [configs_ objectForKey:appKey];
  if (config != nil) {
    if (config.hasConfig) {
      if ([delegate respondsToSelector:@selector(adWhirlConfigDidReceiveConfig:)]) {
        // Don't call directly, instead schedule it in the runloop. Delegate
        // may expect the message to be delivered out-of-band
        [(NSObject *)delegate performSelectorOnMainThread:@selector(adWhirlConfigDidReceiveConfig:)
                                               withObject:config
                                            waitUntilDone:NO];
      }
      return config;
    }
    // If there's already a config fetching, and another call to this function
    // add a delegate to the config
    [config addDelegate:delegate];
    return config;
  }

  // No config, create one, and start fetching it
  return [self fetchConfig:appKey delegate:delegate];
}

- (AdWhirlConfig *)fetchConfig:(NSString *)appKey
                      delegate:(id <AdWhirlConfigDelegate>)delegate {
  AdWhirlConfig *config = [[[AdWhirlConfig alloc] initWithAppKey:appKey
                                                        delegate:delegate]
                           autorelease];
  [self fetchDataForConfig:config];

  [configs_ setObject:config forKey:appKey];
  return config;
}

- (void)dealloc {
  [reachability_ release];
  [connection_ release];
  [receivedData_ release];
  [configs_ release];
  [super dealloc];
}

#pragma mark reachability methods

- (void)fetchDataForConfig:(AdWhirlConfig *)config {

  if (fetchingConfig_ != nil) {
    AWLogDebug(@"Another fetch is in progress, wait until finished.");
    return;
  }
  fetchingConfig_ = config;

  AWLogDebug(@"Checking if config is reachable at %@", config.configURL);

  [reachability_ release];
  reachability_
    = [AWNetworkReachabilityWrapper reachabilityWithHostname:[config.configURL host]
                                            callbackDelegate:self];
  if (reachability_ == nil) {
    [fetchingConfig_ notifyDelegatesOfFailure:
     [AdWhirlError errorWithCode:AdWhirlConfigConnectionError
                     description:
      @"Error setting up reachability check to config server"]];
    return;
  }
  [reachability_ retain];

  if (![reachability_ scheduleInCurrentRunLoop]) {
    [fetchingConfig_ notifyDelegatesOfFailure:
     [AdWhirlError errorWithCode:AdWhirlConfigConnectionError
                     description:
      @"Error scheduling reachability check to config server"]];
    [reachability_ release], reachability_ = nil;
    return;
  }
}

- (void)reachabilityBecameReachable:(AWNetworkReachabilityWrapper *)reach {
  if (reach != reachability_) {
    AWLogError(@"Unrecognized reachability object");
    return;
  }
  // done with the reachability
  [reachability_ release], reachability_ = nil;

  // go fetch config
  NSURLRequest *configRequest
    = [NSURLRequest requestWithURL:fetchingConfig_.configURL];
  connection_ = [[NSURLConnection alloc] initWithRequest:configRequest
                                               delegate:self];
  if (connection_) {
    receivedData_ = [[NSMutableData alloc] init];
  }
  else {
    [fetchingConfig_ notifyDelegatesOfFailure:
     [AdWhirlError errorWithCode:AdWhirlConfigConnectionError
                     description:
      @"Error creating connection to config server"]];
    fetchingConfig_ = nil;
  }
}

#pragma mark NSURLConnection delegate methods.

- (void)connection:(NSURLConnection *)conn didReceiveResponse:(NSURLResponse *)response {
  if (conn != connection_) {
    AWLogError(@"Unrecognized connection object %s:%d", __FILE__, __LINE__);
    return;
  }
  if ([response isKindOfClass:[NSHTTPURLResponse class]]) {
    NSHTTPURLResponse *http = (NSHTTPURLResponse*)response;
    const int status = [http statusCode];

    if (status < 200 || status >= 300) {
      AWLogDebug(@"AdWhirlConfig: HTTP %d, cancelling %@", status, [http URL]);
      [connection_ cancel];
      [fetchingConfig_ notifyDelegatesOfFailure:
       [AdWhirlError errorWithCode:AdWhirlConfigStatusError
                       description:@"Config server did not return status 200"]];
      [connection_ release], connection_ = nil;
      [receivedData_ release], receivedData_ = nil;
      fetchingConfig_ = nil;
      return;
    }
  }

  [receivedData_ setLength:0];
}

- (void)connection:(NSURLConnection *)conn didFailWithError:(NSError *)error {
  if (conn != connection_) {
    AWLogError(@"Unrecognized connection object %s:%d", __FILE__, __LINE__);
    return;
  }
  [fetchingConfig_ notifyDelegatesOfFailure:
   [AdWhirlError errorWithCode:AdWhirlConfigConnectionError
                   description:@"Error connecting to config server"
               underlyingError:error]];
  [connection_ release], connection_ = nil;
  [receivedData_ release], receivedData_ = nil;
  fetchingConfig_ = nil;
}

- (void)connectionDidFinishLoading:(NSURLConnection *)conn {
  if (conn != connection_) {
    AWLogError(@"Unrecognized connection object %s:%d", __FILE__, __LINE__);
    return;
  }
  [fetchingConfig_ parseConfig:receivedData_ error:nil];
  [connection_ release], connection_ = nil;
  [receivedData_ release], receivedData_ = nil;
  fetchingConfig_ = nil;

  ///// TODO: look for the next config to fetch
}

- (void)connection:(NSURLConnection *)conn didReceiveData:(NSData *)data {
  if (conn != connection_) {
    AWLogError(@"Unrecognized connection object %s:%d", __FILE__, __LINE__);
    return;
  }
  [receivedData_ appendData:data];
}

@end

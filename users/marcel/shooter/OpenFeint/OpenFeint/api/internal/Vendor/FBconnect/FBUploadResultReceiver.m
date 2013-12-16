/*
 * Copyright 2009-2010 Facebook
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

#import "FBUploadResultReceiver.h"

@implementation FBUploadResultReceiver

@synthesize _uploadLock,
  _uploadResult,
  _uploadError;

NSAutoreleasePool *_pool;

- (FBUploadResultReceiver *) init {
  if (nil == [super init]) {
    return nil;
  }
  _uploadError = nil;
  return self;
}

- (void) dealloc {
  [_uploadLock release];
  [super dealloc];
}


- (void)requestWithParams:(NSDictionary *)params {
  [NSThread detachNewThreadSelector:@selector(requestAndReceiveResult:)
                           toTarget:self
                         withObject:params];
  // This will block--it will acquire the lock as soon as possible
  [_uploadLock lockWhenCondition:1];

  // To avoid warning that we're deallocating a lock in use
  [_uploadLock unlock];
}

- (void)requestAndReceiveResult:(NSDictionary *)params {
  // Because this will be executing in a new thread
  _pool = [[NSAutoreleasePool alloc] init];
  
  _uploadLock = [[NSConditionLock alloc] initWithCondition:0];
  [_uploadLock lockWhenCondition:0];
  
  [[FBRequest requestWithDelegate:self]
   call:@"facebook.intl.uploadNativeStrings" params:params];
  
  [[NSRunLoop currentRunLoop] run];
}

// FBRequestDelegate protocol

- (void)request:(FBRequest*)request didLoad:(id)result {
  _uploadResult = *((int *)result);
  [_uploadLock unlockWithCondition:1];
  [_pool release];
}

- (void)request:(FBRequest*)request didFailWithError:(NSError*)error {
  NSLog(@"in request didFail");
  _uploadError = error;
  _uploadResult = -1;
  [_uploadLock unlockWithCondition:1];
  [_pool release];
}

@end

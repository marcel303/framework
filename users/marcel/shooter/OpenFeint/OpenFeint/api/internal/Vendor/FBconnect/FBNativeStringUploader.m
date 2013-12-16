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

#import "FBNativeStringUploader.h"
#import "FBUploadResultReceiver.h"

@implementation FBNativeStringUploader

+ (int) uploadStringSet:(NSDictionary *)dictionary
          error:(NSError **)error {
  if (nil == dictionary) {
    [NSException raise:@"NilDictionaryException" 
          format:@"dictionary was nil."];
  }

  NSMutableString *query = [NSMutableString stringWithString:@"["];
  int size = [dictionary count];
  int remaining = size;

  // Build up the JSON for the uploadNativeStrings API:
  // format is [{"text" : "Hello!", "description" : "Greeting."},{...}...]
  for (NSString *string in dictionary) {
    NSString *description = [dictionary objectForKey:string];
    [query appendFormat:@"{\"text\" : \"%@\", \"description\" : \"%@\"}",
              string, description];
    (--remaining > 0) ? 
      [query appendString:@", "] : [query appendString:@"]"];
  }

  FBUploadResultReceiver *requestResultReceiver = 
    [[[FBUploadResultReceiver alloc] init] autorelease];

  [requestResultReceiver requestWithParams:
    [NSDictionary dictionaryWithObject:query forKey:@"native_strings"]];

  *error = requestResultReceiver._uploadError;
  return requestResultReceiver._uploadResult;
}

@end

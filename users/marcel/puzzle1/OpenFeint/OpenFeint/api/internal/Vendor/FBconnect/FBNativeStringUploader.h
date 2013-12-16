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
#import "FBRequest.h"

NSString* FBLocalizedString(NSString *key, NSString *comment);

@interface FBNativeStringUploader : NSObject<FBRequestDelegate> {

}
  
/**  
 * Uploads a set of strings.  
 * @param NSDictionary * dictionary contains the nativeString->comment pairs 
 *                                   to be uploaded 
 * @param NSError **     error      a reference to an error object where an
 *                                  error will be stored if encountered  
 */
+ (int) uploadStringSet:(NSDictionary *)dictionary error:(NSError **)error;
@end

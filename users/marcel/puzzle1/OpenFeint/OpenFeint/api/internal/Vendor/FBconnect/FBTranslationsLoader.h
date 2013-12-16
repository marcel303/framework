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

#import "FBConnectGlobal.h"
#import "FBRequest.h"

NSString* FBLocalizedString(NSString *key, NSString *comment);

@protocol FBTranslationsLoaderDelegate

- (void) translationsDidLoad;
- (void) translationsDidFailWithError:(NSError *)error;

@end

/**
 * FBTranslationsLoader fetches the application's FBT translations.
 */
@interface FBTranslationsLoader : NSObject<FBRequestDelegate> {
}

/**
 * Will load translations for a given locale.
 * @param NSString * locale the Apple (not FBT) locale translations are needed 
 *                          for
 * @param NSObject<FBTranslationsDelegate> *
 *   delegate to be notified when the request has loaded
 */
+ (void) loadTranslationsForLocale:(NSString *)locale 
            delegate:(NSObject<FBTranslationsLoaderDelegate> *)delegate;

/**
 * Will load translations for an automatically detected locale.
 * @param NSObject<FBTranslationsLoaderDelegate> * 
 *   loadDelegate to be notified when the request has loaded
 */
+ (void) loadTranslations:(NSObject<FBTranslationsLoaderDelegate> *)delegate;


/**
 * Returns true if the provided string maps to a supported Facebook locale.
 * @param NSString *	locale	the locale to look up
 */
+ (BOOL) supportsLocale:(NSString *)locale;

@end

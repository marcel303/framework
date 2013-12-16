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

#import "FBTranslationsLoader.h"

NSMutableDictionary* _strings = nil;
NSArray* _identicalLocales = nil;
NSDictionary* _appleToFbLocales = nil;
NSObject<FBTranslationsLoaderDelegate> *_loadDelegate = nil;
FBTranslationsLoader* _sharedInstance = nil;

typedef enum {
  kRequestNotInit,
  kRequestMade,
  kRequestLoaded
} RequestStatus;

RequestStatus _requestStatus = kRequestNotInit;

@implementation FBTranslationsLoader

+ (FBTranslationsLoader*) sharedInstance {
  @synchronized(self) {
    if (nil == _sharedInstance)
      _sharedInstance = [[FBTranslationsLoader alloc] init];
  }
  return _sharedInstance;
}

+ (void) initialize {
  if (nil != _identicalLocales) {
    return;
  }
  
  NSURL *url = 
    [NSURL URLWithString:
      @"http://www.facebook.com/translations/AppleToFbLocales.plist"];
  _appleToFbLocales = [[NSMutableDictionary alloc] 
                       initWithContentsOfURL:url];
}

- (FBTranslationsLoader *) init {
  if (nil == [super init]) {
    return nil;
  }
  
  return self;
}

- (void) release {
  [_strings release];
}

- (void) executeRequest:(NSString *)locale {
  _requestStatus = kRequestMade;
  NSDictionary *params = [NSDictionary dictionaryWithObject:locale 
                                                     forKey:@"locale"];
  [[FBRequest requestWithDelegate:self] call:@"facebook.intl.getTranslations" 
                                      params:params];
}

+ (NSString *) getFbtLocaleFor:(NSString *)appleLocale {
  NSDictionary *dict = [_appleToFbLocales objectForKey:appleLocale];
  if (nil == dict) {
    return nil;
  }
  return [dict objectForKey:@"fbtLocale"];
}

+ (BOOL) supportsLocale:(NSString *)locale {
	if (nil == locale ||
      [locale isEqualToString:@""] ||
      ![self getFbtLocaleFor:locale]) {
    return NO;
  }
  return YES;
}

- (void) loadTranslationsForLocale:(NSString *)locale
                          delegate:(NSObject<FBTranslationsLoaderDelegate> *)
                            delegate {
  if (nil == locale ||
      [locale isEqualToString:@""] ||
       ![FBTranslationsLoader getFbtLocaleFor:locale]) {
    [NSException raise:@"NoLocaleException"
                format:@"Locale was empty string, nil, not recognized,\
 or could not be detected."];
  }
  locale = [FBTranslationsLoader getFbtLocaleFor:locale];
  _loadDelegate = delegate;
  [self executeRequest:locale];
}

/*
 * Loads the translated strings from Facebook for the provided locale.
 * 
 */
+ (void) loadTranslationsForLocale:(NSString *)locale 
                          delegate:(NSObject<FBTranslationsLoaderDelegate> *)
                            delegate {
  [[FBTranslationsLoader sharedInstance] loadTranslationsForLocale:locale
                                                          delegate:delegate];
}

- (NSString *) detectLocale {
  NSString* locale = [[NSLocale currentLocale] 
                      objectForKey: NSLocaleIdentifier];

  return locale;
}

- (void) loadTranslations:(NSObject<FBTranslationsLoaderDelegate> *)delegate {
  _loadDelegate = delegate;
  
  [self loadTranslationsForLocale:[self detectLocale] delegate:delegate];
}

/*
 * Loads the translated strings from Facebook for the automatically detected 
 * locale.
 * 
 */
+ (void) loadTranslations:(NSObject<FBTranslationsLoaderDelegate> *)delegate {
  [[FBTranslationsLoader sharedInstance] loadTranslations:delegate];
}

- (NSString *) getKeyForNativeString:(NSString *)nativeString
                         description:(NSString *)description {
  return [NSString stringWithFormat:@"%@ %@", nativeString, description];
}

- (void) request:(FBRequest *)request didLoad:(id)result {
  NSArray *localeDatas = (NSArray *)result;
  
  NSArray* strings = ([(NSDictionary *) [localeDatas objectAtIndex:0] 
              objectForKey:@"strings"]);
  
  if (_strings != nil) {
    [_strings release];
  }
  _strings = [[NSMutableDictionary dictionary] retain];
  
  for (NSDictionary* dict in strings) {
    NSString* string = [dict objectForKey:@"native_string"];
    for (NSDictionary *nativeStringDict in 
         [dict objectForKey:@"translations"]) {
      NSString* desc = [nativeStringDict objectForKey:@"description"];
      NSString* key;
      key = [self getKeyForNativeString:string description:desc];

      [_strings setObject:[nativeStringDict objectForKey:@"translation"] 
                   forKey:key];
    }
  }

  _requestStatus = kRequestLoaded;
  [_loadDelegate translationsDidLoad];
}

- (NSString *) getTranslationFor:(NSString *)string
                     description:(NSString *)description {
  // Check that the translations have loaded
  if (kRequestLoaded != _requestStatus) {
    [NSException raise:@"TranslationsNotLoadedException"
                format:@"Translations have not been loaded yet. See \
     loadTranslations:, loadTranslationsForLocale:, and your \
     FBTranslationsLoaderDelegate implementation."];
  }

  NSString *translation =
    [_strings objectForKey:[self getKeyForNativeString:string
                                           description:description]];

  if (nil == translation) {
    return string;
  }
  
  return translation;
}

- (NSString *) stringWithString:(NSString *)key  comment:(NSString *)comment {
  if (nil == key || nil == comment) {
    [NSException raise:@"NilArgumentException"
                format:@"Key or comment was null."];
  }
  return [NSString stringWithString:[self getTranslationFor:key
                                                description:comment]];
}

- (void) request:(FBRequest *)request didFailWithError:(NSError *)error {
  _requestStatus = kRequestNotInit;
  [_loadDelegate translationsDidFailWithError:error];
}

@end

NSString* FBLocalizedString(NSString *key, NSString *comment) {
  return [[FBTranslationsLoader sharedInstance] stringWithString:key
                                                         comment:comment];
}

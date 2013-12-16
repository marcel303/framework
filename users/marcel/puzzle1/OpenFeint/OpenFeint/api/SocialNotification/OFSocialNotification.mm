//  Copyright 2009-2010 Aurora Feint, Inc.
// 
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//  
//  	http://www.apache.org/licenses/LICENSE-2.0
//  	
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.

#import "OFSocialNotification.h"

@implementation OFSocialNotification

@synthesize text;
@synthesize imageType;
@synthesize imageIdentifier;
@synthesize imageUrl;
@synthesize url;

-(id)initWithText:(NSString*)_text imageType:(NSString*)_imageType imageIdentifier:(NSString*)_imageIdentifier linkedUrl:(NSString*)_url
{
	self = [super init];
	if (self != nil)
	{
		self.text = _text;
		self.imageType = _imageType;
		self.imageIdentifier = _imageIdentifier;
		self.imageUrl = nil;
		self.url = _url;
	}
	return self;
}

-(id)initWithText:(NSString*)_text imageNamed:(NSString*)_imageName linkedUrl:(NSString*)_url
{
    _url = ([_url length] > 0) ? _url : nil;
	return [self initWithText:_text imageType:@"notification_images" imageIdentifier:_imageName linkedUrl:_url];
}

-(id)initWithText:(NSString*)_text imageNamed:(NSString*)_imageName
{
	return [self initWithText:_text imageType:@"notification_images" imageIdentifier:_imageName linkedUrl:nil];
}

-(id)initWithText:(NSString*)_text
{
	return [self initWithText:_text imageType:nil imageIdentifier:nil linkedUrl:nil];
}

-(id)initWithText:(NSString*)_text imageType:(NSString*)_imageType imageId:(NSString*)_imageId
{
	return [self initWithText:_text imageType:_imageType imageIdentifier:_imageId linkedUrl:nil];
}

-(id)initWithText:(NSString*)_text imageType:(NSString*)_imageType imageId:(NSString*)_imageId linkedUrl:(NSString*)_url
{
    _url = ([_url length] > 0) ? _url : nil;
    return [self initWithText:_text imageType:_imageType imageIdentifier:_imageId linkedUrl:_url];
}

- (void)dealloc
{
	self.text = nil;
	self.imageType = nil;
	self.imageIdentifier = nil;
	self.imageUrl = nil;
	self.url = nil;
	[super dealloc];
}

@end

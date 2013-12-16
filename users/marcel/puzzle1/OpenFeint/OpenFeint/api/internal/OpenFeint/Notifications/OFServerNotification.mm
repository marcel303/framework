//
//  OFServerNotification.mm
//  OpenFeint
//
//  Created by Ron Midthun on 3/1/10.
//  Copyright 2010 __MyCompanyName__. All rights reserved.
//

#import "OFServerNotification.h"
#import "OFResourceDataMap.h"
#import "OFPointer.h"

@implementation OFServerNotification

@synthesize text, detail, backgroundDefaultImage, backgroundImage;
@synthesize iconDefaultImage, iconImage, statusDefaultImage, statusImage;
@synthesize score, inputTab, inputControllerName, showDefaultNotification;

#define makeSetter(ivarName, setterName) -(void) setterName:(NSString*) _text {\
OFSafeRelease(ivarName);\
ivarName = [_text retain];\
}


makeSetter(text, setText);
makeSetter(detail, setDetail);
makeSetter(backgroundDefaultImage, setBackgroundDefaultImage);
makeSetter(backgroundImage, setBackgroundImage);
makeSetter(iconDefaultImage, setIconDefaultImage);
makeSetter(iconImage, setIconImage);
makeSetter(statusDefaultImage, setStatusDefaultImage);
makeSetter(statusImage, setStatusImage);
makeSetter(inputTab, setInputTab);
makeSetter(inputControllerName, setInputControllerName);
makeSetter(showDefaultNotification, setShowDefaultNotification);
makeSetter(score, setScore);

+ (OFResourceDataMap*)getDataMap
{
	static OFPointer<OFResourceDataMap> dataMap;
	
	if(dataMap.get() == NULL)
	{
		dataMap = new OFResourceDataMap;
		dataMap->addField(@"showDefaultNotification", @selector(setShowDefaultNotification:));
        dataMap->addField(@"text", @selector(setText:));
        dataMap->addField(@"detail", @selector(setDetail:));
        dataMap->addField(@"backDefaultImage", @selector(setBackgroundDefaultImage:));
        dataMap->addField(@"backImage", @selector(setBackgroundImage:));
        dataMap->addField(@"iconDefaultImage", @selector(setIconDefaultImage:));
        dataMap->addField(@"iconImage", @selector(setIconImage:));
        dataMap->addField(@"statusDefaultImage", @selector(setStatusDefaultImage:));
        dataMap->addField(@"statusImage", @selector(setStatusImage:));
        dataMap->addField(@"score", @selector(setScore:));
		dataMap->addField(@"inputTab", @selector(setInputTab:));
		dataMap->addField(@"inputControllerName", @selector(setInputControllerName:));
	}
	
	return dataMap.get();
}

+ (NSString*)getResourceName
{
	return @"server_notification";
}

+ (NSString*)getResourceDiscoveredNotification
{
	return @"openfeint_server_notification_received";
}

-(void)dealloc 
{
	OFSafeRelease(showDefaultNotification);
    OFSafeRelease(text);
    OFSafeRelease(detail);
    OFSafeRelease(backgroundDefaultImage);
    OFSafeRelease(backgroundImage);
    OFSafeRelease(iconDefaultImage);
    OFSafeRelease(iconImage);
    OFSafeRelease(statusDefaultImage);
    OFSafeRelease(statusImage);
	OFSafeRelease(inputTab);
	OFSafeRelease(inputControllerName);
	OFSafeRelease(score);
    [super dealloc];
}

@end

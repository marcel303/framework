//
//  OFServerNotificationView.mm
//  OpenFeint
//
//  Created by Phillip Saindon on 3/4/10.
//  Copyright 2010 Aurora Feint Inc. All rights reserved.
//

#import "OFDelegate.h"
#import "OFServerNotification.h"
#import "OFControllerLoader.h"
#import "OFImageView.h"
#import "OFImageLoader.h"
#import "OFInputResponseOpenDashboard.h"
#import "OFViewHelper.h"
#import "OpenFeint+Private.h"

#import "OFServerNotificationView.h"

@interface OFServerNotificationView()

- (void) _loadImagesWithNotification:(OFServerNotification*)serverNotification;
- (void) _updateInputResponse:(OFServerNotification*)serverNotification;
- (void) _updateText:(OFServerNotification*)serverNotification;
- (void) _updateNumber:(OFServerNotification*)serverNotification;

- (void) _setImageViewImage:(OFImageView*)imageView
	 withBackupDefaultImage:(NSString*)backupDefaultImageName
	   withDefaultImageName:(NSString*)defaultImageName 
			andDesiredImage:(NSString*)desiredImageUrl;
- (void) _imageFinishedDownloading;

@end

@implementation OFServerNotificationView

@synthesize icon, twoLineTopLabel, twoLineBottomLabel, numberLabel, smallFeintIconForNumber;

+ (NSString*)notificationViewName
{
	return @"ServerNotificationView";
}

+ (void)showServerNotification:(OFServerNotification*)serverNotification inView:(UIView*)containerView
{
	if(serverNotification == nil)
	{
		//No notification...
		return;
	}
	
	//We are going to show a server notification, make sure to clear out the default notice if there is one so it doesn't
	//get fired off.
	[[OFNotification sharedInstance] clearDefaultNotice];
	
	//keep the server notification around until we use it in configureServerNotification.
	[serverNotification retain];
	
	//Setup configureSeverNotification to be called in the main loop with the serverNotification.
	OFServerNotificationView* view = (OFServerNotificationView*)OFControllerLoader::loadView([self notificationViewName]);
		
	//ensuring thread-safety by firing the notice on the main thread
	SEL selector = @selector(configureServerNotification: inView:);
	NSMethodSignature* methodSig = [view methodSignatureForSelector:selector];
	NSInvocation* invocation = [NSInvocation invocationWithMethodSignature:methodSig];
	[invocation setTarget:view];
	[invocation setSelector:selector];
	[invocation setArgument:&serverNotification atIndex:2];
	[invocation setArgument:&containerView atIndex:3];
	[[NSRunLoop mainRunLoop] addTimer:[NSTimer timerWithTimeInterval:0.f invocation:invocation repeats:NO] forMode:NSDefaultRunLoopMode];
}

- (void)configureServerNotification:(OFServerNotification*)serverNotification inView:(UIView*)containerView
{	
	[self _setPresentationView:containerView];

	//Don't let the view show when setting up
	settingUpView = YES;

	[self _loadImagesWithNotification:serverNotification];
	[self _updateInputResponse:serverNotification];
	[self _updateText:serverNotification];
	[self _updateNumber:serverNotification];
	
	settingUpView = NO;
	
	//If all our url views were cached, or we don't have any views to download, present here.
	if(miDownloadsNeeded == miDownloadsDone)
	{
		[self _presentForDuration:4.0f];
	}
	
	[serverNotification release];
}

- (void) _loadImagesWithNotification:(OFServerNotification*)serverNotification
{
	//Default the background image incase we got a bad one from the server Notification.
	
	static const NSString* DEFAULT_BACKGROUND = @"OFNotificationBackground.png";
	static const NSString* DEFAULT_STATUS_INDICATOR = @"OpenFeintStatusIconNotificationFailure.png";
	static const NSString* DEFAULT_ICON = @"OFProfileIconDefaultSelf.png";
	
	backgroundImage.unframed = YES;
	statusIndicator.unframed = YES;
	icon.unframed = YES;
	
	//Setup images, and count the downloads needed.
	[self _setImageViewImage:backgroundImage
	  withBackupDefaultImage:DEFAULT_BACKGROUND
		withDefaultImageName:serverNotification.backgroundDefaultImage 
			 andDesiredImage:serverNotification.backgroundImage];
	
	[self _setImageViewImage:statusIndicator 
	  withBackupDefaultImage:DEFAULT_STATUS_INDICATOR
		withDefaultImageName:serverNotification.statusDefaultImage 
			 andDesiredImage:serverNotification.statusImage];
	
	[self _setImageViewImage:icon 
	  withBackupDefaultImage:DEFAULT_ICON
		withDefaultImageName:serverNotification.iconDefaultImage
			 andDesiredImage:serverNotification.iconImage];
}

- (void) _updateInputResponse:(OFServerNotification*)serverNotification
{
	//Get the input Response setup based on what the server notification says.
	if((serverNotification.inputTab && ![serverNotification.inputTab isEqualToString:@""])  &&
	   (serverNotification.inputControllerName && ![serverNotification.inputControllerName isEqualToString:@""]))
	{
		//We have a input respose to go to a dashboard page.  Lets setup the input response (and retain it).
		mInputResponse = [[OFInputResponseOpenDashboard alloc] 
						 initWithTab:serverNotification.inputTab
						 andControllerName:serverNotification.inputControllerName];
		
		disclosureIndicator.hidden = NO;
	}
	else
	{
		disclosureIndicator.hidden = YES;
	}
	
}

- (void) _updateText:(OFServerNotification*)serverNotification
{
	OFAssert(!(serverNotification.detail && !serverNotification.text), 
			 "The Server Notification specified a detail with no text, to submit a detail text must be submitted");
	
	//Just Hide all to start at turn on appropriate feilds.
	notice.hidden = YES;
	twoLineTopLabel.hidden = YES;
	twoLineBottomLabel.hidden = YES;
	
	if(serverNotification.text && !serverNotification.detail)
	{
		//One line format
		notice.text = serverNotification.text;
		notice.hidden = NO;
	}
	else if(serverNotification.text && serverNotification.detail)
	{
		//Two line format
		twoLineTopLabel.text = serverNotification.text;
		twoLineBottomLabel.text = serverNotification.detail;
		twoLineTopLabel.hidden = NO;
		twoLineBottomLabel.hidden = NO;
	}
	//else no text and everything stays hidden.
}

- (void) _updateNumber:(OFServerNotification*)serverNotification
{
	if(serverNotification.score && ![serverNotification.score isEqualToString:@""])
	{
		numberLabel.text = serverNotification.score;
	}
	else 
	{
		numberLabel.hidden = YES;
		smallFeintIconForNumber.hidden = YES;
	}

}

- (void) _setImageViewImage:(OFImageView*)imageView
	 withBackupDefaultImage:(NSString*)backupDefaultImageName
	   withDefaultImageName:(NSString*)defaultImageName 
			andDesiredImage:(NSString*)desiredImageUrl
{
	if(!imageView)
	{
		//Sorry, can't do much for you...
		return;
	}
	
	//Are there even image names for us to look at?
	if(defaultImageName || desiredImageUrl)
	{
		//You must give me SOMETHING to fall back on if the server said we need to put something up!
		//This is the "last resort" if this user's current version of open feint doesn't have the image
		//that the server set us as a backup, this garentee's we put up something in the users bundle.
		
		OFAssert(backupDefaultImageName, "No Backup Default Name Specified in code");
		UIImage* backupDefaultImage = nil;
		backupDefaultImage = [OFImageLoader loadImage:backupDefaultImageName];
		OFAssert(backupDefaultImage, "No Image matching Backup Default Name");
		[imageView setDefaultImage:backupDefaultImage];
		
		//If we got either on of these then we need to load the default while the desired is being loaded
		if(defaultImageName)
		{
			UIImage* image = nil;
			image = [OFImageLoader loadImage:defaultImageName];
			if(image)
			{
				[imageView setDefaultImage:[image stretchableImageWithLeftCapWidth:(backgroundImage.image.size.width - 18) topCapHeight:0]];
				[imageView setContentMode:UIViewContentModeScaleToFill];
			}
		}
		//Else either we don't have a default image from the server, or the default image didn't load.
		//The default image may not load because this may be an old version of Open Feint and they may
		//not have the default image in their bundle.  For these cases the Xib defines a backup default
		//image which is already initalized to the image.  We'll use this if we need it.
		
		if(desiredImageUrl && ![desiredImageUrl isEqualToString:@""])
		{
			miDownloadsNeeded++;
			
			//Set the desired image to download and update us when its done and showing.
			OFDelegate showAndCountDelegate(self, @selector(_imageFinishedDownloading));
			[imageView setImageDownloadFinishedDelegate:showAndCountDelegate];
			imageView.imageUrl = desiredImageUrl;
		}
	}
	else 
	{
		imageView.hidden = YES;
	}

}

- (void)_imageFinishedDownloading
{
	miDownloadsDone++;
	if(miDownloadsDone == miDownloadsNeeded)
	{
		if(!settingUpView)
		{
			[self _presentForDuration:4.0f];
			
			//clear out all the delegates we might have set now that we are all downloaded.
			[backgroundImage setImageDownloadFinishedDelegate:OFDelegate()];
			[statusIndicator setImageDownloadFinishedDelegate:OFDelegate()];
		}
	}
}

@end

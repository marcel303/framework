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

#import "OFPlayerBannerCell.h"
#import "OFImageLoader.h"
#import "OFUser.h"
#import "OFResource.h"
#import "OFImageView.h"

#import "OpenFeint+Settings.h"
#import "OpenFeint+Private.h"

@implementation OFPlayerBannerCell

@synthesize user;

- (void)onResourceChanged:(OFResource*)resource
{
	// XXX todo ugh
	float kfNameFontSize = 15.f;
	float kfPlayingFontSize = 12.f;
	float kfFeintPtsFontSize = 10.f;
	if ([OpenFeint isLargeScreen])
	{
		kfNameFontSize = 17.f;
		kfPlayingFontSize = 14.f;
		kfFeintPtsFontSize = 12.f;
	}
	
	// scrub all existing views
	[bgView removeFromSuperview];
	bgView = nil;
	[feintPtsView removeFromSuperview];
	feintPtsView = nil;
	[separatorView removeFromSuperview];
	separatorView = nil;
	[profileView removeFromSuperview];
	profileView = nil;
	[feintPtsLabel removeFromSuperview];
	feintPtsLabel = nil;
	[playerNameLabel removeFromSuperview];
	playerNameLabel = nil;
	[nowPlayingLabel removeFromSuperview];
	nowPlayingLabel = nil;
	[onlineStatusLabel removeFromSuperview];
	onlineStatusLabel = nil;

	[user release];
	user = [(OFUser*)resource retain];
	
	if (!user)
	{
		UIImage* bgImg = [OFImageLoader loadImage:([OpenFeint isInLandscapeMode] ? @"OFNowPlayingBannerOffline.png" : @"OFNowPlayingBannerOffline.png")];
		bgView = [[UIImageView alloc] initWithImage:bgImg];
		[self addSubview:bgView];
		[bgView release];		
	}
	else
	{
		UIImage* bgImg = [[OFImageLoader loadImage:@"OFPlayerBanner.png"] stretchableImageWithLeftCapWidth:10 topCapHeight:21];
		bgView = [[[UIImageView alloc] initWithImage:bgImg] autorelease];
		[self addSubview:bgView];
		
		UIImage* feintPtsImg = [OFImageLoader loadImage:@"OFFeintPointsWhite.png"];
		feintPtsView = [[[UIImageView alloc] initWithImage:feintPtsImg] autorelease];
		[self addSubview:feintPtsView];
		
		UIImage* separatorImg = [[OFImageLoader loadImage:@"OFPlayerFeintPts.png"] stretchableImageWithLeftCapWidth:0 topCapHeight:1];
		separatorView = [[[UIImageView alloc] initWithImage:separatorImg] autorelease];
		[self addSubview:separatorView];

		profileView = [[[OFImageView alloc] init] autorelease];
		[profileView useProfilePictureFromUser:user];
		[self addSubview:profileView];
		[profileView addTarget:self action:@selector(profilePictureTouched) forControlEvents:UIControlEventTouchUpInside];
		
		feintPtsLabel = [[UILabel alloc] init];
		feintPtsLabel.backgroundColor = [UIColor clearColor];
		feintPtsLabel.textColor = [UIColor whiteColor];	
		feintPtsLabel.font = [UIFont boldSystemFontOfSize:kfFeintPtsFontSize];
		feintPtsLabel.text = [NSString stringWithFormat:@"%u", user.gamerScore];
		[self addSubview:feintPtsLabel];
		
		playerNameLabel = [[UILabel alloc] init];
		playerNameLabel.backgroundColor = [UIColor clearColor];
		playerNameLabel.textColor = [UIColor whiteColor];	
		playerNameLabel.shadowColor = [UIColor blackColor];
		playerNameLabel.shadowOffset = CGSizeMake(0, -1);
		playerNameLabel.font = [UIFont boldSystemFontOfSize:kfNameFontSize];
		playerNameLabel.text = user.name;
		[self addSubview:playerNameLabel];
		
		nowPlayingLabel = [[UILabel alloc] init];
		nowPlayingLabel.backgroundColor = [UIColor clearColor];
		nowPlayingLabel.textColor = [UIColor colorWithWhite:0.8 alpha:1.0];	
		nowPlayingLabel.shadowColor = [UIColor blackColor];
		nowPlayingLabel.shadowOffset = CGSizeMake(0, -1);
		nowPlayingLabel.font = [UIFont fontWithName:@"Helvetica-BoldOblique" size:kfPlayingFontSize];
		
		if ([user isLocalUser])
		{
			nowPlayingLabel.text = [NSString stringWithFormat:@"Playing %@", user.lastPlayedGameName];
		}
		else if ([user online]) 
		{
			nowPlayingLabel.text = [NSString stringWithFormat:@"playing %@", user.lastPlayedGameName];
            onlineStatusLabel = [[UILabel alloc] init];
            onlineStatusLabel.font =  [UIFont boldSystemFontOfSize:kfPlayingFontSize];
            onlineStatusLabel.backgroundColor = [UIColor clearColor];
            onlineStatusLabel.textColor = [UIColor colorWithRed:90.0/255.0 green:180.0/255.0 blue:90.0/255.0 alpha:1.0];
            onlineStatusLabel.shadowColor = [UIColor blackColor];
            onlineStatusLabel.shadowOffset = CGSizeMake(0, -1);
			onlineStatusLabel.text = @"ONLINE";
		} 
		else 
		{
			nowPlayingLabel.text = [NSString stringWithFormat:@"Last played %@", user.lastPlayedGameName];
		}
		
		[self addSubview:nowPlayingLabel];
		
		if (onlineStatusLabel)
		{
			[self addSubview:onlineStatusLabel];
		}
	}
	
	[self layoutSubviews];
}

- (void)layoutSubviews
{
	// XXX todo ugh
	float kfSeparatorPadLeft = 28.f;
	float kfFeintPtsPadLeft = 19.f;
	float kfFeintPtsPadRight = 15.f;
	float kfProfilePadLeft = 3.f;
	float kfProfilePadTop = 3.f;
	float kfLabelPadLeft = 9.f;
	float kfOnlineStatusIconPadTop = 18.f;
	float kfOnlineStatusIconPadLeft = 8.f;
	float kfNameLabelPadBottom = -4.f;
	float kfPlayingLabelPadTop = 2.f;
	CGSize kProfileSize = CGSizeMake(36.f, 36.f);
	if ([OpenFeint isLargeScreen])
	{
        kfSeparatorPadLeft = 38.f;
		kfNameLabelPadBottom = 3.f;
        kfPlayingLabelPadTop = -2.f;
		kfOnlineStatusIconPadTop = 20.f;
		kfProfilePadTop = 6.f;
		kfProfilePadLeft = 1.f;
		kProfileSize = CGSizeMake(50.f, 50.f);
	}

	CGRect frame = self.frame;
	frame.origin = CGPointZero;
	
	bgView.contentMode = UIViewContentModeScaleToFill;
	bgView.frame = frame;

	if (feintPtsLabel)
	{
		feintPtsLabel.contentMode = UIViewContentModeLeft;
		CGSize textSize = [feintPtsLabel.text sizeWithFont:feintPtsLabel.font];
		CGFloat labelWidth = kfFeintPtsPadRight + textSize.width;
		feintPtsLabel.frame = CGRectMake(frame.size.width - labelWidth, 0.f,
									labelWidth, frame.size.height);
		
		profileView.contentMode = UIViewContentModeCenter;
		profileView.shouldScaleImageToFillRect = YES;
        
        CGRect profileViewFrame = CGRectMake(kfProfilePadLeft, kfProfilePadTop,
                                             kProfileSize.width, kProfileSize.height);
        if ([OpenFeint isLargeScreen]) profileViewFrame.origin.x += 5;
        
		profileView.frame = profileViewFrame;
		profileView.unframed = YES;

		[frameView removeFromSuperview];
		frameView = nil;

		// This is somewhat of a hack.
		// Our superview is an OFFramedContentWrapperView.
		// His superview is an OFBannerFrame.  Putting the frame into that view
		// will allow us to animate with the banner and render on top of the
		// border frame.
		// This will be null until about the third time we get layout,
		// but at that point, it'll work.
		UIView* frameOwner = self.superview.superview;
		if (frameOwner)
		{
			// XXX todo also don't like this!
			UIImage* frameImage = [OFImageLoader loadImage:[OpenFeint isLargeScreen] ? @"OFPlayerFrameIPad.png" : @"OFPlayerFrame.png"];
			frameView = [[UIImageView alloc] initWithImage:frameImage];
			[frameOwner addSubview:frameView];
			[frameView release];
			frameView.contentMode = UIViewContentModeScaleToFill;
			CGPoint center = [self convertPoint:profileView.center toView:frameOwner];
			center.x = roundf(center.x);
			center.y = roundf(center.y);
			frameView.center = center;
		}
		
		CGFloat frameViewMaxX = 0.f;
		if (frameView)
			frameViewMaxX = CGRectGetMaxX(frameView.frame);
			
		if (onlineStatusLabel)
		{
			CGFloat statusOrigin = frameViewMaxX + kfOnlineStatusIconPadLeft;
			onlineStatusLabel.frame = CGRectMake(statusOrigin, (frame.size.height - kfOnlineStatusIconPadTop - 14.0)/2.0 + kfOnlineStatusIconPadTop,
                                                 55.0, 14.0);
		}
		
		CGFloat playerNameOrigin = frameViewMaxX + kfLabelPadLeft;
		
		playerNameLabel.contentMode = UIViewContentModeCenter;
		CGSize playerNameSize = [playerNameLabel.text sizeWithFont:playerNameLabel.font];
		playerNameLabel.frame = CGRectMake(playerNameOrigin, frame.size.height/2.0 - playerNameSize.height - kfNameLabelPadBottom,
										   playerNameSize.width, playerNameSize.height);

		CGFloat labelOrigin = kfLabelPadLeft;
		if (onlineStatusLabel)
			labelOrigin += CGRectGetMaxX(onlineStatusLabel.frame);
		else if (frameView)
			labelOrigin += frameViewMaxX;

		nowPlayingLabel.contentMode = UIViewContentModeCenter;
		CGSize nowPlayingSize = [nowPlayingLabel.text sizeWithFont:nowPlayingLabel.font];
		nowPlayingLabel.frame = CGRectMake(labelOrigin - (onlineStatusLabel ? 5 : 0), frame.size.height/2.0 + kfPlayingLabelPadTop,
										   nowPlayingSize.width, nowPlayingSize.height);
		
		separatorView.contentMode = UIViewContentModeScaleToFill;
		separatorView.frame = CGRectMake(frame.size.width - (labelWidth + kfSeparatorPadLeft), 0,
										separatorView.image.size.width, frame.size.height);

		feintPtsView.contentMode = UIViewContentModeLeft;
		CGFloat imageWidth = labelWidth + kfFeintPtsPadLeft;
		feintPtsView.frame = CGRectMake(frame.size.width - imageWidth, 0,
										imageWidth, frame.size.height);
	}
	
	[self bringSubviewToFront:feintPtsLabel];
}

- (void) profilePictureTouched
{
	if ([bannerProvider respondsToSelector:@selector(bannerProfilePictureTouched)])
	{
		[bannerProvider performSelector:@selector(bannerProfilePictureTouched)];
	}
	else
	{
		[bannerProvider onBannerClicked];
	}
}

- (void) removeFromSuperview
{
	[frameView removeFromSuperview];
	frameView = nil;
	[super removeFromSuperview];
}

@end

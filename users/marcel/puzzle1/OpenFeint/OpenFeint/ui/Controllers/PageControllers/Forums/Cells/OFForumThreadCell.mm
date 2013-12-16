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

#import "OFForumThreadCell.h"
#import "OFForumThread.h"

#import "OFUser.h"
#import "OFImageView.h"
#import "OFImageLoader.h"


@implementation OFForumThreadCell

- (void)dealloc
{
	OFSafeRelease(titleLabel);
	OFSafeRelease(stickyLabel);
	OFSafeRelease(postsCountLabel);
	OFSafeRelease(lastPostByLabel);
	OFSafeRelease(favoritedIcon);
	OFSafeRelease(threadStatusIcon);
	[super dealloc];
}

- (BOOL)hasConstantHeight
{
	return YES;
}

- (void)onResourceChanged:(OFResource*)resource
{
	OFForumThread* thread = (OFForumThread*)resource;

	float const kRightPad = 40.f;
	
	CGRect frame = titleLabel.frame;
	frame.origin.x = thread.isSticky ? CGRectGetMaxX(stickyLabel.frame) : CGRectGetMinX(stickyLabel.frame);
	frame.size.width = self.frame.size.width - frame.origin.x - kRightPad;
	titleLabel.frame = frame;
	titleLabel.text = thread.title;

	lastPostByLabel.text = [NSString stringWithFormat:@"Last post by %@", thread.lastPostAuthor.name];

	postsCountLabel.text = [NSString stringWithFormat:@"%d", thread.postCount];
	postsCountLabel.hidden = thread.isLocked;
	threadStatusIcon.hidden = !postsCountLabel.hidden;
	favoritedIcon.hidden = !thread.isSubscribed;
	stickyLabel.hidden = !thread.isSticky;
}

@end

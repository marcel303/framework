////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// 
///  Copyright 2009 Aurora Feint, Inc.
/// 
///  Licensed under the Apache License, Version 2.0 (the "License");
///  you may not use this file except in compliance with the License.
///  You may obtain a copy of the License at
///  
///  	http://www.apache.org/licenses/LICENSE-2.0
///  	
///  Unless required by applicable law or agreed to in writing, software
///  distributed under the License is distributed on an "AS IS" BASIS,
///  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
///  See the License for the specific language governing permissions and
///  limitations under the License.
/// 
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#import "OFDefaultTextView.h"
#import "OFImageLoader.h"
#include "OFBase.h"

@implementation OFDefaultTextView

- (void)didMoveToSuperview
{
	if (!bgView)
	{
		bgView = [[UIImageView alloc] initWithImage:[[OFImageLoader loadImage:@"OFTextViewFrame.png"] stretchableImageWithLeftCapWidth:7 topCapHeight:7]];
		bgView.frame = self.frame;
	}
	[self.superview insertSubview:bgView belowSubview:self];
}

- (void)removeFromSuperview
{
	[super removeFromSuperview];
	[bgView removeFromSuperview];
}

- (void)setFrame:(CGRect)_frame
{
	[super setFrame:_frame];
	[bgView setFrame:_frame];
}

- (void)dealloc
{
	OFSafeRelease(bgView);
	[super dealloc];
}

@end

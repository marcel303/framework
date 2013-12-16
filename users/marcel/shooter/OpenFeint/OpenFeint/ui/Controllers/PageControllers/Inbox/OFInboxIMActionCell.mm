////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// 
///  Copyright 2010 Aurora Feint, Inc.
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

#import "OFInboxIMActionCell.h"
#import "OFImageLoader.h"

@implementation OFInboxIMActionCell
@synthesize inlayView;

-(void)addInlay {
    if(self.inlayView) {
        return;
    }
	
	
	
    UIImage *image = [[OFImageLoader loadImage:@"OFIMActionInlay.png"] stretchableImageWithLeftCapWidth:15 topCapHeight:35];
	self.inlayView = [[[UIImageView alloc] initWithImage:image] autorelease];
    self.inlayView.frame = self.frame;
    self.inlayView.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    [self.contentView addSubview:self.inlayView];
    [self.contentView sendSubviewToBack:self.inlayView];
}


-(void)awakeFromNib {
    [self addInlay];
}
/*
- (void)setSelected:(BOOL)selected animated:(BOOL)animated {

    [super setSelected:selected animated:animated];

    // Configure the view for the selected state
}*/


- (void)dealloc {
    self.inlayView = nil;
    [super dealloc];
}


@end

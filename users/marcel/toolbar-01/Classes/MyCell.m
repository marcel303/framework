//
//  MyCell.m
//  toolbar-01
//
//  Created by Marcel Smit on 01-04-10.
//  Copyright 2010 Apple Inc. All rights reserved.
//

#import "MyCell.h"


@implementation MyCell

- (id)initWithStyle:(UITableViewCellStyle)style reuseIdentifier:(NSString *)reuseIdentifier {
    if ((self = [super initWithStyle:style reuseIdentifier:reuseIdentifier])) {
		UILabel* label = [[UILabel alloc] initWithFrame:CGRectMake(5.0f, 5.0f, 280.0f, 50.0f)];
		[label setText:@"Hello!\nTest"];
		[label setTextAlignment:UITextAlignmentCenter];
		[label setNumberOfLines:2];
		[self.contentView addSubview:label];
		[label release];
		
		UITextView* text = [[UITextView alloc] initWithFrame:CGRectMake(5.0f, 5.0f, 310.0f, 30.0f)];
		[text setText:@"Hello\nWorld!"];
		[text setTextAlignment:UITextAlignmentCenter];
		[text setAutoresizingMask:UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight];
//		[self.contentView addSubview:text];
		[text release];
    }
    return self;
}

- (void)setSelected:(BOOL)selected animated:(BOOL)animated {

    [super setSelected:selected animated:animated];

    // Configure the view for the selected state
}


- (void)dealloc {
    [super dealloc];
}


@end

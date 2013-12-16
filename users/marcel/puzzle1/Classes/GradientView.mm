//
//  GradientView.mm
//  puzzle1
//
//  Created by Marcel Smit on 10-09-10.
//  Copyright 2010 __MyCompanyName__. All rights reserved.
//

#import <QuartzCore/QuartzCore.h>
#import "GradientView.h"


@implementation GradientView

//- (id)initWithFrame:(CGRect)frame {
-(id)initWithCoder:(NSCoder *)aDecoder
{
//    if ((self = [super initWithFrame:frame])) {
	if ((self = [super initWithCoder:aDecoder]))
	{
        // Initialization code
		
	 CAGradientLayer *gradient = [CAGradientLayer layer];
	 gradient.frame = self.bounds;
	 gradient.colors =
		[NSArray arrayWithObjects:(id)[[UIColor blackColor] CGColor], (id)[[UIColor colorWithRed:0.3f green:0.35f blue:0.4f alpha:1.0f] CGColor], (id)[[UIColor blackColor] CGColor], nil];
	 [self.layer insertSublayer:gradient atIndex:0];
    }
    return self;
}

/*-(void)drawRect:(CGRect)rect 
{	
}*/

- (void)dealloc {
    [super dealloc];
}


@end

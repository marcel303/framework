//
//  UISlider.m
//  paint-01
//
//  Created by Marcel Smit on 07-06-09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#import "UIVerticalSlider.h"

@implementation UIVerticalSlider

@synthesize text;
@synthesize value;
@synthesize valueChanged;
@synthesize m_Target;

- (id)initWithFrame:(CGRect)frame target:(NSObject*)target {
	frame = CGRectMake(frame.origin.x, frame.origin.y, 30.0f, CGRectGetHeight(frame));
	
    if (self = [super initWithFrame:frame]) {
		[self awakeFromNib];
    }
	
    return self;
}

- (void)awakeFromNib
{
	[super awakeFromNib];
	
	// Initialization code
//	self.text = @"Text";
//	self.value = 1.0f;
//	self.m_Target = target;
	
	self.userInteractionEnabled = TRUE;
}

- (void)setValue:(float)v
{
	value = v;

	NSLog(@"ValueChanged A");
	
	if (valueChanged)
	{
		NSLog(@"ValueChanged B");
		
		[m_Target performSelector:valueChanged];
	}
	
	[self setNeedsDisplay];
}

- (void)HandleTouch:(UITouch*)touch
{
	CGPoint location = [touch locationInView:self];
	
	float v = 1.0f - location.y / (self.frame.size.height - 1.0f);
	
	if (v < 0.0f)
		v = 0.0f;
	if (v > 1.0f)
		v = 1.0f;
	
	[self setValue:v];
}

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
	NSLog(@"TouchBegin");
	
	[self HandleTouch:[touches anyObject]];
}

- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{
	[self HandleTouch:[touches anyObject]];
}

- (void)drawRect:(CGRect)rect {
    // Drawing code
	CGContextRef context = UIGraphicsGetCurrentContext();
	float col_Back[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	CGContextSetFillColor(context, col_Back);
	CGContextFillRect(context, rect);

	float col_Text[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	CGContextSetFillColor(context, col_Text);
	
	float size = 14.0f;
	UIFont* font = [UIFont fontWithName:@"Arial" size:size];
	
	CGContextSaveGState(context);
	CGContextRotateCTM(context, M_PI / 2.0f);
	CGContextTranslateCTM(context, 0.0f, -size);
	
	[text drawAtPoint:CGPointMake(0.0f, 0.0f) withFont:font];
	
	CGContextRestoreGState(context);
	
    CGContextSetLineWidth(context, 1.0f);
    CGContextSetStrokeColorWithColor(context, [UIColor whiteColor].CGColor);
    CGContextSetFillColorWithColor(context, [UIColor blackColor].CGColor);
	CGContextSetAlpha(context, 1.0f);
	
    CGRect rrect = CGRectMake(size + 2.0f, 0.0f, 10.0f, CGRectGetHeight(self.frame));
    
    CGFloat radius = 3.0f;
    CGFloat width = CGRectGetWidth(rrect);
    CGFloat height = CGRectGetHeight(rrect);
    
    // Make sure corner radius isn't larger than half the shorter side
    if (radius > width/2.0)
        radius = width/2.0;
    if (radius > height/2.0)
        radius = height/2.0;    
    
    CGFloat minx = CGRectGetMinX(rrect) + 1.0f;
    CGFloat midx = CGRectGetMidX(rrect);
    CGFloat maxx = CGRectGetMaxX(rrect) - 1.0f; // todo: use stroke size.
    CGFloat miny = CGRectGetMinY(rrect) + 1.0f;
    CGFloat midy = CGRectGetMidY(rrect);
    CGFloat maxy = CGRectGetMaxY(rrect) - 1.0f;
	CGContextBeginPath(context);
    CGContextMoveToPoint(context, minx, midy);
    CGContextAddArcToPoint(context, minx, miny, midx, miny, radius);
    CGContextAddArcToPoint(context, maxx, miny, maxx, midy, radius);
    CGContextAddArcToPoint(context, maxx, maxy, midx, maxy, radius);
    CGContextAddArcToPoint(context, minx, maxy, minx, midy, radius);
    CGContextClosePath(context);
    CGContextDrawPath(context, kCGPathFillStroke);
	
    CGContextSetFillColorWithColor(context, [UIColor greenColor].CGColor);
    rrect = CGRectMake(size + 2.0f, CGRectGetHeight(self.frame) - value * CGRectGetHeight(self.frame), 10.0f, value * CGRectGetHeight(self.frame));
    minx = CGRectGetMinX(rrect) + 1.0f;
    midx = CGRectGetMidX(rrect);
    maxx = CGRectGetMaxX(rrect) - 1.0f; // todo: use stroke size.
    miny = CGRectGetMinY(rrect) + 1.0f;
    midy = CGRectGetMidY(rrect);
    maxy = CGRectGetMaxY(rrect) - 1.0f;
	CGContextBeginPath(context);
    CGContextMoveToPoint(context, minx, midy);
    CGContextAddArcToPoint(context, minx, miny, midx, miny, radius);
    CGContextAddArcToPoint(context, maxx, miny, maxx, midy, radius);
    CGContextAddArcToPoint(context, maxx, maxy, midx, maxy, radius);
    CGContextAddArcToPoint(context, minx, maxy, minx, midy, radius);
    CGContextClosePath(context);
    CGContextDrawPath(context, kCGPathFillStroke);
}

- (void)dealloc {
    [super dealloc];
}

@end

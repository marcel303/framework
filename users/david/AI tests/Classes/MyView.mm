//
//  MyView.m
//  AI tests
//
//  Created by Narf on 6/23/09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#import "MyView.h"

#include "AITest.h"
#include "World.h"
#include "Rendering.h"

double stront;

@implementation MyView

bool HoleBuilding = false;
int x,y;
float rad = 0.0f;
void UpdatePreviewHole()
{
	if(HoleBuilding)
	{
		double stront2 = CFAbsoluteTimeGetCurrent() - stront;
		stront = CFAbsoluteTimeGetCurrent();
		rad += stront2*10;
		
	}
}

void RenderPreviewHole()
{
	if(HoleBuilding)
	{
		RenderCircle(x, y, rad);
	}
}

- (void)touchesBegan:(NSSet*)_touches withEvent:(UIEvent*)event
{
	NSSet* touches = [event allTouches];
	
	for (int i = 0; i < touches.count; ++i)
	{
		UITouch* touch = [[touches allObjects] objectAtIndex:i];
		
		CGPoint location = [touch locationInView:self];
		
		x = location.x;
		y = location.y;
		rad = 10.0f;
	}
	
	stront = CFAbsoluteTimeGetCurrent();
	HoleBuilding = true;
}

- (void)touchesMoved:(NSSet*)_touches withEvent:(UIEvent*)event
{
	NSSet* touches = [event allTouches];
	for (int i = 0; i < touches.count; ++i)
	{

		UITouch* touch = [[touches allObjects] objectAtIndex:i];
	
		CGPoint location = [touch locationInView:self];
	
		x = location.x;
		y = location.y;
	}
	
}

- (void)touchesEnded:(NSSet*)_touches withEvent:(UIEvent*)event
{
	World::I().PlaceBlackHole(x, y, rad);
	
	HoleBuilding = false;
}



-(void)initialize
{
	// Initialization code
	
	[self setOpaque:TRUE];
	
	[NSTimer scheduledTimerWithTimeInterval:0.002f target:self selector:@selector(HandleTimer) userInfo:NULL repeats:YES];
	
	[self setNeedsDisplay];
}


- (id)initWithFrame:(CGRect)frame {
    if (self = [super initWithFrame:frame]) {
		[self initialize];
    }
    return self;
}

-(id)initWithCoder:(NSCoder*)coder
{
	if (self = [super initWithCoder:coder])
	{
		[self initialize];
	}
	
	return self;
}

+(void)Circle:(int) x:(int)y:(int)w
{
	CGContextRef context = UIGraphicsGetCurrentContext();
	CGContextStrokeEllipseInRect(context, CGRectMake(x,y,w,w));
}

- (void)drawRect:(CGRect)rect {
    // Drawing code
	//CGContextRef context = UIGraphicsGetCurrentContext();
	
	//CGContextSetRGBFillColor(context, 255, 0, 0, 1.0);
	//CGContextFillRect(context, CGRectMake(0, 0, 100, 100));
	//CGContextStrokeEllipseInRect(context, CGRectMake(50,50,32,32));
	//CGContextFillEllipseInRect(context, CGRectMake(0, 0, 32, 32));
	World::I().Render();
	RenderPreviewHole();
	//World::I().Update();
	
	
		
}

- (void)HandleTimer
{
	[self setNeedsDisplay];
	
	World::I().Update();
	
	UpdatePreviewHole();
	//
}




- (void)dealloc {
    [super dealloc];
}


@end

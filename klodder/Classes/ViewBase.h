#pragma once
#import <UIKit/UIView.h>

#define FONT_TYPWRITER @"AmericanTypewriter"
#define FONT_MARKER @"Marker Felt"

@interface ViewBase : UIView 
{

}

-(void)handleFocus;
-(void)handleFocusLost;
-(void)drawFrameBorder;

extern UILabel* CreateLabel(float x, float y, float sx, float sy, NSString* text, NSString* font, float fontSize, UITextAlignment alignment);
extern UIToolbar* CreateToolBar();

@end

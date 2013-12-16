#import <UIKit/UIView.h>
#import "klodder_forward_objc.h"

@interface View_Gauge : UIView 
{
	id delegate;
	SEL changed;
	SEL provideText;
	
	int min;
	int max;
	int value;
	bool enabled;
	NSString* unitText;
	
	View_Gauge_Button* less;
	View_Gauge_Button* more;
	View_Gauge_ActiveRegion* activeRegion;
	UILabel* label;
}

@property (assign) int value;
@property (readonly, assign) float fill;
@property (assign) bool enabled;

-(id)initWithLocation:(CGPoint)location scale:(float)scale height:(float)height min:(int)min max:(int)max value:(int)value unit:(NSString*)unitText delegate:(id)delegate changed:(SEL)changed provideText:(SEL)provideText;
-(void)setValueDirect:(int)value;
-(float)fill;
-(int)fillToValue:(float)fill;
-(void)setEnabled:(bool)enabled;

@end

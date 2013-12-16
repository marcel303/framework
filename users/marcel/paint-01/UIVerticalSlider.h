#import <UIKit/UIKit.h>


@interface UIVerticalSlider : UIView {
	NSString* text;
	float value;
	SEL valueChanged;
	IBOutlet NSObject* m_Target;
}

@property(retain) NSString* text;
@property float value;
@property SEL valueChanged;
@property(retain) NSObject* m_Target;

- (id)initWithFrame:(CGRect)frame target:(NSObject*)target;

@end

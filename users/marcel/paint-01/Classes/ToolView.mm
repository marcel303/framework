#import "ToolView.h"

@implementation ToolView

- (void)awakeFromNib
{
	self.exclusiveTouch = TRUE;
	self.multipleTouchEnabled = FALSE;
	self.userInteractionEnabled = TRUE;
	self.opaque = TRUE;
	
#if 0
	float sliderX = 20.0f;
	float sliderY = 100.0f;
	float sliderHeight = 100.0f;
#endif
	
#if 0
	sliderColorHue = [[UIVerticalSlider alloc] initWithFrame:CGRectMake(sliderX + 0.0f, sliderY, 0.0f, sliderHeight) target:self];
	sliderColorSat = [[UIVerticalSlider alloc] initWithFrame:CGRectMake(sliderX + 40.0f, sliderY, 0.0f, sliderHeight) target:self];
	sliderColorVal = [[UIVerticalSlider alloc] initWithFrame:CGRectMake(sliderX + 80.0f, sliderY, 0.0f, sliderHeight) target:self];
	sliderColorOpac = [[UIVerticalSlider alloc] initWithFrame:CGRectMake(sliderX + 120.0f, sliderY, 0.0f, sliderHeight) target:self];
	sliderBrushSize = [[UIVerticalSlider alloc] initWithFrame:CGRectMake(sliderX + 160.0f, sliderY, 0.0f, sliderHeight) target:self];
	sliderBrushHard = [[UIVerticalSlider alloc] initWithFrame:CGRectMake(sliderX + 200.0f, sliderY, 0.0f, sliderHeight) target:self];
#endif
	
	[sliderColorHue setText:@"Hue"];
	[sliderColorSat setText:@"Saturation"];
	[sliderColorVal setText:@"Value"];
	[sliderColorOpac setText:@"Opacity"];
	[sliderBrushSize setText:@"Size"];
	[sliderBrushHard setText:@"Hardness"];
	
	sliderColorHue.valueChanged = @selector(sliderChanged);
	sliderColorSat.valueChanged = @selector(sliderChanged);
	sliderColorVal.valueChanged = @selector(sliderChanged);
	sliderColorOpac.valueChanged = @selector(sliderChanged);
	sliderBrushSize.valueChanged = @selector(sliderChanged_Brush);
	sliderBrushHard.valueChanged = @selector(sliderChanged_Brush);
	
	[sliderColorHue setValue:0.5f];
	[sliderColorSat setValue:0.5f];
	[sliderColorVal setValue:1.0f];
	[sliderColorOpac setValue:1.0f];
	[sliderBrushSize setValue:0.5f];
	[sliderBrushHard setValue:0.5f];
	
	[self updateColor];
	[self updateBrush];
	
#if 0
	[self addSubview:sliderColorHue];
	[self addSubview:sliderColorSat];
	[self addSubview:sliderColorVal];
	[self addSubview:sliderColorOpac];
	[self addSubview:sliderBrushSize];
	[self addSubview:sliderBrushHard];
#endif
}

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
}

- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{
}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
}

- (void)drawRect:(CGRect)rect {
	self.opaque = NO; // todo: move to init.
	super.backgroundColor = [UIColor clearColor];
	
    CGContextRef context = UIGraphicsGetCurrentContext();
    CGContextSetLineWidth(context, 1.0f);
    CGContextSetStrokeColorWithColor(context, [UIColor whiteColor].CGColor);
    CGContextSetFillColorWithColor(context, [UIColor blackColor].CGColor);
	CGContextSetAlpha(context, 0.5f);
	
    CGRect rrect = self.bounds;
    
    CGFloat radius = 20.0f;
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
    CGContextMoveToPoint(context, minx, midy);
    CGContextAddArcToPoint(context, minx, miny, midx, miny, radius);
    CGContextAddArcToPoint(context, maxx, miny, maxx, midy, radius);
    CGContextAddArcToPoint(context, maxx, maxy, midx, maxy, radius);
    CGContextAddArcToPoint(context, minx, maxy, minx, midy, radius);
    CGContextClosePath(context);
    CGContextDrawPath(context, kCGPathFillStroke);
}

- (void)setBackgroundColor:(UIColor *)newBGColor
{
	// Ignore.
}
- (void)setOpaque:(BOOL)newIsOpaque
{
	// Ignore.
}

- (void)updateColor
{
	float hue = sliderColorHue.value;
	float sat = sliderColorSat.value;
	float val = sliderColorVal.value;
//	float hue = sliderColorHue.value;
//	float sat = sliderColorSat.value;
//	float val = sliderColorVal.value;
	
	viewColor.backgroundColor = [UIColor colorWithHue:hue saturation:sat brightness:val alpha:1.0f];
	
	CGColorRef color = viewColor.backgroundColor.CGColor;
	
	const float* components = CGColorGetComponents(color);
	
	rgb[0] = components[0];
	rgb[1] = components[1];
	rgb[2] = components[2];
	
	opacity = sliderColorOpac.value;
}

- (void)updateBrush
{
	// todo: create newly sized brush.
	
	size = sliderBrushSize.value * 80.0f;
	hardness = sliderBrushHard.value;
}

- (void)sliderChanged
{
	[self updateColor];
}

- (void)sliderChanged_Brush
{
	[self sliderChanged];
	
	[self updateBrush];
}

/*

- (void)sliderColorHueChange:(id)sender
{
	[self updateColor];
}
- (void)sliderColorSatChange:(id)sender
{
	[self updateColor];
}
- (void)sliderColorValChange:(id)sender
{
	[self updateColor];
}

- (void)sliderSizeChange:(id)sender
{
	size = sliderSize.value;
}

- (void) sliderOpacityChange:(id)sender
{
	opacity = sliderOpacity.value;
}
*/

-(IBAction) buttonToolBrushClick:(id)sender
{
	toolType = Paint::ToolType_Brush;
}
-(IBAction) buttonToolSmudgeClick:(id)sender
{
	toolType = Paint::ToolType_Smudge;
}
-(IBAction) buttonToolSoftenClick:(id)sender
{
	toolType = Paint::ToolType_Smoothe;
}

- (void)dealloc {
    [super dealloc];
}

@end

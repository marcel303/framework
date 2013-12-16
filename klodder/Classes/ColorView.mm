#if 0

#import "ColorView.h"

#import "Calc.h"
#import "MacImage.h"
#import "Types.h"
#define SIZE 255

@implementation ColorView

- (id)initWithFrame:(CGRect)frame
{
    if ((self = [super initWithFrame:frame])) 
	{
		[self setOpaque:FALSE];
		
		mMacImage = [self createImage];
		
		mImage = mMacImage->Image_get();
		
#if 0
		UIImage* image = [UIImage imageWithCGImage:mImage];
		
//		NSData* data = UIImageJPEGRepresentation(image, 1.0);
		NSData* data = UIImagePNGRepresentation(image);

		NSArray* paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
		NSString* documentsDirectory = [paths objectAtIndex:0];
		NSString* appFile = [documentsDirectory stringByAppendingPathComponent:@"colors.png"];
		[data writeToFile:appFile atomically:YES];
		
		UIImage* image2 = [[UIImage alloc] initWithData:data];

		UIImageWriteToSavedPhotosAlbum(image2, nil, nil, 0);
		
//		UIImageWriteToSavedPhotosAlbum(image, nil, nil, 0);
#endif
    }
	
    return self;
}

- (void)drawRect:(CGRect)rect
{
	return;
	
	CGContextRef ctx = UIGraphicsGetCurrentContext();
	CGContextSaveGState(ctx);
//	CGContextSetBlendMode(ctx, kCGBlendModeCopy);
//	CGContextTranslateCTM(ctx, 0.0f, SIZE);
//	CGContextScaleCTM(ctx, 1.0, -1.0);
	CGRect r = CGRectMake(0.0f, 0.0f, SIZE, SIZE);
	CGContextDrawImage(ctx, r, mImage);
	CGContextRestoreGState(ctx);
}

- (void)dealloc
{
    [super dealloc];
}

-(MacImage*)createImage
{
	MacImage* image = new MacImage();
	
	image->Size_set(SIZE, SIZE, false);
	
	Vec2F mid((SIZE - 1) / 2, (SIZE - 1) / 2);
	int radius = (SIZE - 1) / 2;
	
	for (int y = 0; y < image->Sy_get(); ++y)
	{
		MacRgba* line = image->Line_get(y);
		
		for (int x = 0; x < image->Sx_get(); ++x)
		{
			Vec2F location(x, y);
			Vec2F delta = location - mid;
			float angle = Vec2F::ToAngle(delta) / Calc::m2PI;
			if (angle < 0.0f)
				angle += 1.0f;
//			NSLog(@"angle: %f", angle);
			float distance = delta.Length_get() / radius;
			
			UIColor* color;
			
			if (distance <= 1.0f)
				color = [UIColor colorWithHue:angle saturation:distance brightness:1.0f alpha:1.0f];
			else
				color = [UIColor colorWithRed:0.0f green:0.0f blue:0.0f alpha:0.0f];
			
			CGColorRef cgColor = [color CGColor];
			
			uint8_t* rgba = line[x].rgba;
			
			const CGFloat* components = CGColorGetComponents(cgColor);
			const float alpha = CGColorGetAlpha(cgColor);
			
			rgba[0] = components[0] * 255.0f;
			rgba[1] = components[1] * 255.0f;
			rgba[2] = components[2] * 255.0f;
			rgba[3] = alpha * 255.0f;
		}
	}
	
	return image;
}

@end

#endif

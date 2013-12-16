#import <AudioToolbox/AudioToolbox.h>
#import <AudioToolbox/AudioQueue.h>
#import "PaintView.h"
#import "TileRenderer.h"
#import "Types.h"

int st = 4; // # bytes per pixel

//

static BoundingBoxI g_InvalidatedRegion;
static TileRenderer* g_TileRenderer;

//

@implementation PaintView

static void HandleDirty(void* _self, void* obj)
{
//	PaintView* self = (PaintView*)_self;
	
	Paint::DirtyEvent* event = (Paint::DirtyEvent*)obj;

	PointI min = PointI(event->x1, event->y);
	PointI max = PointI(event->x2, event->y);
	
	g_InvalidatedRegion.Merge(min);
	g_InvalidatedRegion.Merge(max);
	
	//g_TileRenderer->MarkAsbDirty(event->x1, event->y, event->x2, event->y);
}

- (id)initWithFrame:(CGRect)frame {
	if (self = [super initWithFrame:frame]) {
		
		[self setOpaque:TRUE];
		[self setMultipleTouchEnabled:TRUE];
		
		// Create tile renderer.
		
//		g_TileRenderer = new TileRenderer(320, 480, 16, 16);
		g_TileRenderer = new TileRenderer(320, 480, 32, 32);
		
		application = new Paint::Application(g_TileRenderer->Bitmap_get());
		application->m_DirtyCB = Paint::CallBack(self, HandleDirty);
				
		toolView = [[[NSBundle mainBundle] loadNibNamed:@"ToolView" owner:self options:nil] objectAtIndex:0];
		[toolView setFrame:CGRectMake((self.frame.size.width - toolView.frame.size.width) / 2, self.frame.size.height - toolView.frame.size.height - 10, toolView.frame.size.width, toolView.frame.size.height)];
		[toolView setHidden:TRUE];
		[self addSubview:toolView];
		
		toolButton = [UIButton buttonWithType:UIButtonTypeInfoDark];
		[toolButton setFrame:CGRectMake(CGRectGetWidth(self.frame) - 30, CGRectGetHeight(self.frame) - 30, 20.0f, 20.0f)];
		[toolButton addTarget:self action:@selector(HandleToolButtonClick) forControlEvents:UIControlEventTouchUpInside]; 
		[self addSubview:toolButton];
	}
	
	[self setNeedsDisplay];
	
	return self;
}

- (void)loadImage:(UIImage*)image
{
	int sx = image.size.width;
	int sy = image.size.height;
	
	if (sx > COLBUF_SX)
		sx = COLBUF_SX;
	if (sy > COLBUF_SY)
		sy = COLBUF_SY;

	// Get image date into byte array
	
	UInt8* bytes = new UInt8[sx * sy * 4];
	
	CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
	CGContextRef ctx = CGBitmapContextCreate(bytes, sx, sy, 8, sx * 4, colorSpace, kCGImageAlphaPremultipliedLast);
	CGColorSpaceRelease(colorSpace);
//	CGContextDrawImage(ctx, CGRectMake(0.0f, 0.0f, image.size.width, image.size.height), image.CGImage);
	CGContextDrawImage(ctx, CGRectMake(0.0f, 0.0f, sx, sy), image.CGImage);
	CGContextRelease(ctx);
	
	// Convert from byte array to color buffer
	
	UInt8* byte = bytes;
	
	for (int y = 0; y < sy; ++y)
	{
		for (int x = 0; x < sx; ++x)
		{
			application->m_ColBuf.m_Lines[y][x].v[0] = INT_TO_FIX(byte[0]);
			application->m_ColBuf.m_Lines[y][x].v[1] = INT_TO_FIX(byte[1]);
			application->m_ColBuf.m_Lines[y][x].v[2] = INT_TO_FIX(byte[2]);
			
			byte += 4;
		}
	}

	delete[] bytes;

//	g_InvalidatedRegion.Merge(PointI(0, 0));
//	g_InvalidatedRegion.Merge(PointI(COLBUF_SX - 1, COLBUF_SY - 1));
	[self emitDrawArea];
}

- (void)HandleToolButtonClick
{
	[toolView setHidden:!toolView.hidden];
}

static void ToXY(NSSet* touches, UIView* view, int& x, int& y)
{
	UITouch* touch = [touches anyObject];
	
	x = [touch locationInView:view].x;
	y = [touch locationInView:view].y;
}

- (void)emitDrawArea
{
	// Make sure dirty area is validated.
	
	application->HandlePaint(application, 0);
	
#if 0
	// Request display for each dirty tile.
	
	for (int tileX = 0; tileX < g_TileRenderer->TileCountX_get(); ++tileX)
	{
		for (int tileY = 0; tileY < g_TileRenderer->TileCountY_get(); ++tileY)
		{
			int index = g_TileRenderer->GetTileIndex(tileX, tileY);
			
			TileRenderer::Tile& tile = g_TileRenderer->m_Tiles[index];
			
			if (!tile.IsDirty)
				continue;
			
			[self setNeedsDisplayInRect:tile.Rect];
		}
	}
#endif
	
	if (g_InvalidatedRegion.IsSet_get())
	{
		CGRect rect = CGRectMake(
			g_InvalidatedRegion.min.x,
			g_InvalidatedRegion.min.y,
			g_InvalidatedRegion.max.x - g_InvalidatedRegion.min.x + 1,
			g_InvalidatedRegion.max.y - g_InvalidatedRegion.min.y + 1);
		
		[self setNeedsDisplayInRect:rect];
	}
}

- (void)updateBrushProperties
{
	application->m_ToolType = toolView->toolType;
	application->m_Col = Paint::Col(toolView->rgb[0], toolView->rgb[1], toolView->rgb[2]);
	application->m_Opacity = REAL_TO_FIX(toolView->opacity);
	if (application->m_Brush.m_Sx != toolView->size || application->m_Brush.m_Hardness != REAL_TO_FIX(toolView->hardness))
	{
		application->m_Brush.MakeCircle(toolView->size, REAL_TO_FIX(toolView->hardness));
		application->UpdateBrush();
	}
}

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
//	AudioServicesPlaySystemSound(kSystemSoundID_Vibrate);
	
	if (toolView.hidden == FALSE)
	{
		toolView.hidden = TRUE;
	}
	else
	{
		[self updateBrushProperties];
		
		Paint::TouchInfo info;
		
		info.m_X = 0;
		info.m_Y = 0;
		info.m_Pressed = true;
		info.m_Pressure = 255;
		
		ToXY(touches, self, info.m_X, info.m_Y);
		
		application->m_InputMgr.EmitTouchEvent(Paint::InputType_TouchBegin, &info);
		
		[self emitDrawArea];
	}
}

// Handles the continuation of a touch.
- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{
	CGRect bounds = [self bounds];
//	UITouch* touch = [[event touchesForView:self] anyObject];
	
//	NSSet* touches = [event touchesForView:self];
	
	Paint::TouchInfo info;
	
	info.m_X = 0;
	info.m_Y = 0;
	info.m_Pressed = true;
	info.m_Pressure = 255;
	
	ToXY(touches, self, info.m_X, info.m_Y);
	
	application->m_InputMgr.EmitTouchEvent(Paint::InputType_TouchMove, &info);
	
#if 0
	// todo: use multitouch (!)
	for (int i = 0; i < touches.count; ++i)
	{
		UITouch* touch = [[touches allObjects] objectAtIndex:i];
		
		int x = [touch locationInView:self].x;
		int y = [touch locationInView:self].y;
		
		if (x >= 0 && y >= 0 && x < g_TileRenderer->Sx_get() && y < g_TileRenderer->Sy_get())
		{
			uint8* pixel = g_TileRenderer->Bitmap_get()->GetPixel(x, y);
			
			pixel[0] = rand();
			pixel[1] = rand();
			pixel[2] = rand();
			
			g_TileRenderer->MarkAsbDirty(x, y, x, y);
		}
	}
#endif

	[self emitDrawArea];
}

// Handles the end of a touch event when the touch is a tap.
- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
	Paint::TouchInfo info;
	
	info.m_X = 100; // todo
	info.m_Y = 100;
	info.m_Pressed = true;
	info.m_Pressure = 255;
	
	ToXY(touches, self, info.m_X, info.m_Y);
	
	application->m_InputMgr.EmitTouchEvent(Paint::InputType_TouchEnd, &info);
	
	[self emitDrawArea];
}

// Handles the end of a touch event.
- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event
{
}

- (void)drawRect:(CGRect)rect
{
	LogMgr::WriteLine(LogLevel_Debug, "drawRect: (%f, %f), (%f, %f)", rect.origin.x, rect.origin.y, rect.size.width, rect.size.height);

	for (int tileX = 0; tileX < g_TileRenderer->TileCountX_get(); ++tileX)
	{
		for (int tileY = 0; tileY < g_TileRenderer->TileCountY_get(); ++tileY)
		{
			const int index = g_TileRenderer->GetTileIndex(tileX, tileY);
			
			TileRenderer::Tile* tile = &g_TileRenderer->m_Tiles[index];

			// todo: check if tile inside rect
			
			if (!CGRectIntersectsRect(tile->Rect, rect))
				continue;
			
//			if (!tile->IsDirty)
//				continue;
			
			// todo: do not draw if tile doesn't overlap requested draw area.
			
#if 0
			
#if 0
			CGImageRef imageRef = CGBitmapContextCreateImage(g_TileRenderer->m_Tiles[index].Context);
#else
			CGImageRef imageRef = g_TileRenderer->m_Tiles[index].Image;
			CGImageRetain(imageRef);
#endif
			
#if 1
			UIImage* image = [[UIImage alloc] initWithCGImage:imageRef];
#if 1
			[image drawAtPoint:CGPointMake(tileX * g_TileRenderer->TileSx_get(), tileY * g_TileRenderer->TileSy_get())];
#else
			float scale = 4.0f;
			CGRect r = CGRectMake(tile->Rect.origin.x * scale, tile->Rect.origin.y * scale, tile->Rect.size.width * scale, tile->Rect.size.height * scale);
			[image drawInRect:r];
#endif
			[image release];
#else
			CGContextRef ctx = UIGraphicsGetCurrentContext();
			CGContextSaveGState(ctx);
			CGContextTranslateCTM(ctx, tile->Rect.origin.x, tile->Rect.origin.y + tile->Rect.size.height);
			CGContextScaleCTM(ctx, 1.0, -1.0);
			CGRect r = CGRectMake(0.0f, 0.0f, tile->Rect.size.width, tile->Rect.size.height);
			CGContextDrawImage(ctx, r, imageRef);
			CGContextRestoreGState(ctx);
#endif			

			CGImageRelease(imageRef);
#endif
			
#if 1
#if 0
			UIImage* image2 = [[UIImage alloc] initWithCGImage:g_TileRenderer->m_Image];
			[image2 drawAtPoint:CGPointMake(0.0f, 0.0f)];
#else
			CGContextRef ctx = UIGraphicsGetCurrentContext();
			CGContextSaveGState(ctx);
			CGContextTranslateCTM(ctx, 0.0f, g_TileRenderer->Sy_get());
			CGContextScaleCTM(ctx, 1.0, -1.0);
			CGRect r = CGRectMake(0.0f, 0.0f, g_TileRenderer->Sx_get(), g_TileRenderer->Sy_get());
			CGContextDrawImage(ctx, r, g_TileRenderer->m_Image);
			CGContextRestoreGState(ctx);
#endif
#endif
			
#if 0
			CGContextRef context = UIGraphicsGetCurrentContext();
			CGContextSetRGBStrokeColor(context, (rand() & 255) / 255.0f, 0.0f, 0.0f, 1.0f);
			CGContextStrokeRect(context, tile->Rect);
#endif
			
			tile->IsDirty = false;
		}
	}
	
	g_InvalidatedRegion.Reset();
}

- (void)dealloc
{
	[timer release];
//	CGImageRelease(imageRef);
//	CGContextRelease(ctxRef);
	[super dealloc];
}

@end

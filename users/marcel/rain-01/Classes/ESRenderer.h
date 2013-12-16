//
//  ESRenderer.h
//  rain-01
//
//  Created by user on 3/26/10.
//  Copyright __MyCompanyName__ 2010. All rights reserved.
//

#if 0

#import <QuartzCore/QuartzCore.h>

#import <OpenGLES/EAGL.h>
#import <OpenGLES/EAGLDrawable.h>

@protocol ESRenderer <NSObject>

- (void) render;
- (BOOL) resizeFromLayer:(CAEAGLLayer *)layer;

@end

#endif
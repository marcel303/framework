#pragma once
#import <Foundation/Foundation.h>

#define EVT_COLOR_CHANGED @"ColorChanged"
#define EVT_LAYERS_CHANGED @"LayersChanged"

@interface Events : NSObject
{

}

+(void)post:(NSString*)name;

@end

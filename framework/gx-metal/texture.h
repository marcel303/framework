#pragma once

#ifdef __OBJC__

#import <map>
#import <Metal/Metal.h>

extern std::map<int, id <MTLTexture>> s_textures;
extern int s_nextTextureId;

#endif
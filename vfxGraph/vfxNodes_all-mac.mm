/*

we create a single source file which includes all of the source files in the vfxNodes directory. this serves two purposes,

1) it makes sure the constructors of the vfx node registrations run. it uses a trick where at the end of the source file it includes a function, which gets called by createVfxGraph. this ensures that the source file and generated object are references eventually as the linker traverses references starting at the main(..) module. it's a long standing issue initializers are only invoked when objects are actually referenced. this ensure they do

2) it speeds up library compilation speed, as all vfx node sources are compiled at once, avoiding the overhead of compiling them separately

*/
#include <GL/glew.h>

#include "vfxNodes_all-mac.h"

void linkVfxNodes_Mac() { }

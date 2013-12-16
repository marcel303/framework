mingw
	install including msys
	add mingw bin and msys bin paths to PATH
	mingw for gcc, etc. msys for rsync, cp

boost
	extract archive
	./bootstrap
	./b2 --with-system --with-filesystem --toolset=gcc
	copy .a files to mingw bin folder, removing release and version from filesnames (so system-mt.a eg)

SDL
	extract to MSVC bin and include (in SDL folder)

sox
	extract sox.exe to usg folder

OpenAL
	run installer
	copy from program files to MSVC bin and include (in OpenAL folder)

ogg/vorbis
	copy lib files to MSVC lib directory
	copy include/ogg and include/vorbis (from the source distribution) to MSVC include

----

build tools

open command prompt
navigate tool checkout location

> set OS=win
> mingw32-make

----

build content

checkout content repository in content_svn

> mingw32-make content PF=win

----

build game

open critwave.sln
build

make sure content is built before building the game
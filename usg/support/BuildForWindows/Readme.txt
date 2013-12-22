NOTE: msvc bin and include files are available in the Critical Wave DATA repository.
	extract the lib and include directories fro 3rdparty/win32.rar into the appropriate MSVC directories
	you can skip most of the steps below for setting up MSVC

mingw:
	install including msys
	add mingw bin and msys bin paths to the global PATH environment variable
	mingw is required for gcc under Windows. msys is required for tools like rsync, cp

boost (built with mingw):
	download the latest version from http://boost.org/
	extract the archive
	open a command line (with mingw and msys bin paths added to PATH) in the extract boost folder
	execute these commands:
	> bootstrap
	> b2 --with-system --with-filesystem --toolset=gcc
	copy the generated .a files to the mingw bin folder, removing release and version from filesnames (so system-mt.a eg)

SDL (msvc):
	extract to MSVC bin and include (into /include/SDL/)
		#include <SDL/SDL.h>

sox:
	extract sox.exe to usg folder

OpenAL (msvc):
	run the installer
	copy from location in program files where the installer puts it, into the
		appropriate MSVC bin and include directories (into /include/OpenAL/)
		#include <OpenAL/OpenAL.h>
		
ogg/vorbis:
	copy lib files to MSVC lib directory
	copy include/ogg and include/vorbis (from the source distribution) to MSVC include

----

build tools

open a command prompt where the code has been checked out
execute these commands:
> set OS=win
> mingw32-make

----

build content

clone repository into /usg/content-hg/
open a command prompt in /usg
execute:
> mingw32-make content PF=win

----

build game

open critwave.sln
build

make sure content is built before building the game
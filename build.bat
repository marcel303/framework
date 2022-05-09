@echo off

where /q cmake
IF ERRORLEVEL 1 (
	echo CMake not found. Please install CMake and make sure to add its location to the system path.
	pause
	exit /b 1
)

rem use command line argument for selecting build target
set target_arg=
:target_loop
	IF [%1]==[] (
		goto target_end
	)
	set target_arg=%target_arg%-target %1 
	shift /1
	goto target_loop
:target_end

IF DEFINED target_arg (
	echo using filter: %target_arg%
)

rem verify all submodules are up to date
echo Updating Git submodules..
git submodule update
echo ..Done!

set chibi_bin="chibi-build/chibi/Debug/chibi.exe"

rem build chibi binary
mkdir "chibi-build\chibi"
cd chibi-build/chibi && cmake -DCMAKE_BUILD_TYPE=Release ../../chibi && cmake --build . || cd %~dp0 && exit /b 1
cd %~dp0 || exit /b 1

rem generate cmake files using chibi
mkdir "chibi-build\cmake-files-for-build"
%chibi_bin% -g . chibi-build/cmake-files-for-build %target_arg% || cd %~dp0 && exit /b 1
cd %~dp0 || exit /b 1

rem build all of the libraries and example and test app binaries. this will take a while
mkdir "chibi-build\bin"
cd chibi-build/bin && cmake -DCMAKE_BUILD_TYPE=Release -A Win32 ../cmake-files-for-build && cmake --build . --parallel 4 || cd %~dp0 && exit /b 1
cd %~dp0 || exit /b 1

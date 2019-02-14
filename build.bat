@echo off

where /q cmake
IF ERRORLEVEL 1 (
	echo CMake not found. Please install CMake and make sure to add its location to the system path.
	pause
	exit /b
)

rem todo : verify all submodules are synced somehow
rem git submodule update --init --recursive

set chibi_bin="chibi-build/chibi/Debug/chibi.exe"

rem build chibi binary
mkdir "chibi-build\chibi"
cd chibi-build/chibi && cmake -DCMAKE_BUILD_TYPE=Release ../../chibi && cmake --build . || cd %~dp0 && exit /b
cd %~dp0 || exit /b

rem generate cmake files using chibi
mkdir "chibi-build\cmake-files"
%chibi_bin% . chibi-build/cmake-files || cd %~dp0 && exit /b
cd %~dp0 || exit /b

rem build all of the libraries and example and test app binaries. this will take a while
mkdir "chibi-build\bin"
cd chibi-build/bin && cmake -DCMAKE_BUILD_TYPE=Release ../cmake-files && cmake --build . || cd %~dp0 && exit /b
cd %~dp0 || exit /b

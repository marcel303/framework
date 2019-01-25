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
cd chibi-build/chibi && cmake -DCMAKE_BUILD_TYPE=Release ../../chibi && cmake --build . || exit /b
cd %~dp0 || exit /b

rem generate cmake files using chibi
mkdir "chibi-build\cmake-files"
%chibi_bin% . chibi-build/cmake-files || exit /b
cd %~dp0 || exit /b

rem generate Visual Studio project file
mkdir "chibi-build\vs2015"
cd chibi-build/vs2015 && cmake -G "Visual Studio 14 2015" ../cmake-files
cd %~dp0 || exit /b

mkdir "chibi-build\vs2017"
cd chibi-build/vs2017 && cmake -G "Visual Studio 15 2017" ../cmake-files
cd %~dp0 || exit /b

%SystemRoot%\explorer.exe /select,.\chibi-build\vs2017\Project.sln
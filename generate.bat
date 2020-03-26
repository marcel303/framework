@echo off

where /q cmake
IF ERRORLEVEL 1 (
	echo CMake not found. Please install CMake and make sure to add its location to the system path.
	pause
	exit /b
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
cd chibi-build/chibi && cmake -DCMAKE_BUILD_TYPE=Debug ../../chibi && cmake --build . || cd %~dp0 && exit /b
cd %~dp0 || exit /b

rem generate cmake files using chibi
mkdir "chibi-build\cmake-files"
%chibi_bin% -g . chibi-build/cmake-files %target_arg% || cd %~dp0 && exit /b
cd %~dp0 || exit /b

rem generate Visual Studio project file

set success="false"

IF %success% == "false" (
	mkdir "chibi-build\vs2019"
	cd chibi-build/vs2019 && cmake -G "Visual Studio 16 2019" -A Win32 ../cmake-files
	IF ERRORLEVEL 1 (
		echo Failed to generate Visual Studio 2019 solution.
	) ELSE (
		set success="true"
		%SystemRoot%\explorer.exe /select,.\Project.sln
	)
	cd %~dp0 || exit /b
)

IF %success% == "false" (
	mkdir "chibi-build\vs2017"
	cd chibi-build/vs2017 && cmake -G "Visual Studio 15 2017" -A Win32 ../cmake-files
	IF ERRORLEVEL 1 (
		echo Failed to generate Visual Studio 2017 solution.
	) ELSE (
		set success="true"
		%SystemRoot%\explorer.exe /select,.\Project.sln
	)
	cd %~dp0 || exit /b
)

IF %success% == "false" (
	mkdir "chibi-build\vs2015"
	cd chibi-build/vs2015 && cmake -G "Visual Studio 14 2015" -A Win32 ../cmake-files
	IF ERRORLEVEL 1 (
		echo Failed to generate Visual Studio 2015 solution.
	) ELSE (
		set success="true"
		%SystemRoot%\explorer.exe /select,.\Project.sln
	)
	cd %~dp0 || exit /b
)

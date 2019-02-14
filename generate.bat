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

rem generate Visual Studio project file

set success="false"

IF %success% == "false" (
	mkdir "chibi-build\vs2017"
	cd chibi-build/vs2017 && cmake -G "Visual Studio 15 2017" ../cmake-files
	IF ERRORLEVEL 1 (
		echo Failed to generate Visual Studio 2017 solution.
	) ELSE (
		set success="true"
	)
	cd %~dp0 || exit /b

	IF %success% == "false" (
		%SystemRoot%\explorer.exe /select,.\chibi-build\vs2017\Project.sln
	)
)

IF %success% == "false" (
	mkdir "chibi-build\vs2015"
	cd chibi-build/vs2015 && cmake -G "Visual Studio 14 2015" ../cmake-files
	IF ERRORLEVEL 1 (
		echo Failed to generate Visual Studio 2015 solution.
	) ELSE (
		set success="true"
	)
	cd %~dp0 || exit /b

	IF %success% == "false" (
		%SystemRoot%\explorer.exe /select,.\chibi-build\vs2015\Project.sln
	)
)

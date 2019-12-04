@echo off

for /f "delims=" %%a in ('wmic OS Get localdatetime  ^| find "."') do set dt=%%a
set DATETIME_STRING=%dt:~0,8%_%dt:~8,6%
rem echo %DATETIME_STRING%

where /q cmake
IF ERRORLEVEL 1 (
	echo CMake not found. Please install CMake and make sure to add its location to the system path.
	pause
	exit /b
)

rem verify all submodules are up to date
echo Updating Git submodules..
git submodule update
echo ..Done!

set chibi_bin="chibi-build/chibi/Debug/chibi.exe"

rem build chibi binary
mkdir "chibi-build\chibi"
cd chibi-build/chibi && cmake -DCMAKE_BUILD_TYPE=Release ../../chibi && cmake --build . || cd %~dp0 && exit /b
cd %~dp0 || exit /b

rem use command line argument for selecting build target
:build_loop
	IF [%1]==[] (
		goto build_end
	)
	
	rem generate cmake files using chibi
	mkdir "chibi-build\cmake-files-for-archive"
	%chibi_bin% -g . chibi-build/cmake-files-for-archive -target %1 || cd %~dp0 && exit /b
	cd %~dp0 || exit /b

	rem build the selected target
	mkdir "chibi-build\archive"
	cd chibi-build/archive && cmake -DCMAKE_BUILD_TYPE=Distribution ../cmake-files-for-archive && cmake --build . --config Distribution || cd %~dp0 && exit /b
	cd %~dp0 || exit /b

	rem zip the target folder
	call zip-folder.bat chibi-build\archive\%1 chibi-build\archive\%1-%DATETIME_STRING%.zip

	rem  open explorer with the generated archive selected
	%SystemRoot%\explorer.exe /select,.\chibi-build\archive\%1-%DATETIME_STRING%.zip

	shift /1
	goto build_loop
:build_end

rem echo build loop done
rem pause

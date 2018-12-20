rem todo : verify all submodules are synced somehow
rem git submodule update --init --recursive

set chibi_bin="chibi-build/chibi/Debug/chibi.exe"

echo %chibi_bin%
echo %~dp0

rem build chibi binary
mkdir "chibi-build\chibi"
cd chibi-build/chibi && cmake -DCMAKE_BUILD_TYPE=Release ../../chibi && cmake --build .
cd %~dp0

rem generate cmake files using chibi
mkdir "chibi-build\cmake-files"
%chibi_bin% . chibi-build/cmake-files -target framework* -target audio*
cd %~dp0

rem generate Visual Studio project file
mkdir "chibi-build\vs2015"
cd chibi-build/vs2015 && cmake -G "Visual Studio 14 2015" ../cmake-files
cd %~dp0

rem build all of the libraries and example and test app binaries. this will take a while
mkdir "chibi-build\bin"
cd chibi-build/bin && cmake -DCMAKE_BUILD_TYPE=Release ../cmake-files && cmake --build .
cd %~dp0

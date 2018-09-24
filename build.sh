# generate cmake files using chibi
mkdir chibi-build/cmake-files
"/Users/thecat/Code Projects/Chibi/Debug/chibi" . chibi-build/cmake-files

# build the binaries
mkdir chibi-build/bin
cd chibi-build/bin && cmake -DCMAKE_BUILD_TYPE=Release ../cmake-files && cmake --build .
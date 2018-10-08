# todo : verify all submodules are synced somehow
# git submodule update --init --recursive

root=$PWD
chibi_bin="./chibi-build/chibi/chibi"

# detect operating system
unameOut="$(uname -s)"
case "${unameOut}" in
    Linux*)     os=linux;;
    Darwin*)    os=mac;;
    *)          os="unknown"
esac

# build chibi binary
mkdir -p chibi-build/chibi
cd chibi-build/chibi && cmake -DCMAKE_BUILD_TYPE=Release ../../chibi && cmake --build .
cd "$root"

# generate cmake files using chibi
mkdir -p chibi-build/cmake-files
"$chibi_bin" . chibi-build/cmake-files

if [ "$os" == "mac" ]; then
	# generate Xcode project file
	mkdir -p chibi-build/xcode
	cd chibi-build/xcode && cmake -G "Xcode" ../cmake-files
	cd "$root"
fi

if [ "$os" == "linux" ]; then
	# generate Unix Makefiles
	mkdir -p chibi-build/makefiles
	cd chibi-build/makefiles && cmake -G "Unix Makefiles" ../cmake-files
	cd "$root"
fi

# build all of the libraries and example and test app binaries. this will take a while
mkdir -p chibi-build/bin
cd chibi-build/bin && cmake -DCMAKE_BUILD_TYPE=Release ../cmake-files && cmake --build .
cd "$root"

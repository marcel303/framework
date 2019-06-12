# todo : verify all submodules are synced somehow
# git submodule update --init --recursive

root=$PWD
chibi_bin="./chibi-build/chibi/chibi"

# end the shell script when an error occurs
set -e

# detect operating system
unameOut="$(uname -s)"
case "${unameOut}" in
    Linux*)     os=linux;;
    Darwin*)    os=mac;;
    *)          os="unknown"
esac

# create "-filter <...>" command line arguments to be passed to chibi if this shell script received any arguments of its own
if [ -z "$1" ]
	then
		echo "missing target name"
		exit 1
fi

target_arg="-target $1"

#tput smul; # underline
tput bold; # bold
#tput setab 0; # bg = black
tput setaf 2; # fg = green
echo "target: $1"
tput sgr0; # reset text formatting

# build chibi binary
mkdir -p chibi-build/chibi
cd chibi-build/chibi && cmake -DCMAKE_BUILD_TYPE=Release ../../chibi && cmake --build .
cd "$root"

# generate cmake files using chibi
mkdir -p chibi-build/cmake-files-for-archive
"$chibi_bin" -g . chibi-build/cmake-files-for-archive $target_arg

# build all of the libraries and example and test app binaries. this will take a while
mkdir -p chibi-build/archive
cd chibi-build/archive && cmake -DCMAKE_BUILD_TYPE=Distribution ../cmake-files-for-archive && cmake --build . -- -j6
cd "$root"

if [ "$os" == "mac" ]; then
	open -a Finder -R "chibi-build/archive/$1.app"
fi

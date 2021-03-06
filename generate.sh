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
target_arg=""
for arg in "$@"
do
    target_arg="$target_arg-target $arg "
done

if [ "$target_arg" != "" ]; then
	#tput smul; # underline
	tput bold; # bold
	#tput setab 0; # bg = black
	tput setaf 2; # fg = green
	echo "using filter: $target_arg"
	tput sgr0; # reset text formatting
fi

# build chibi binary
mkdir -p chibi-build/chibi
cd chibi-build/chibi && cmake -DCMAKE_BUILD_TYPE=Release ../../chibi && cmake --build . --config Release
cd "$root"

# generate cmake files using chibi
mkdir -p chibi-build/cmake-files
"$chibi_bin" -g . chibi-build/cmake-files $target_arg

if [ "$os" == "mac" ]; then
	# generate Xcode project file
	mkdir -p chibi-build/xcode
	cd chibi-build/xcode && cmake -G "Xcode" ../cmake-files
	cd "$root"

	open -a Finder -R chibi-build/xcode/Project.xcodeproj
fi

if [ "$os" == "linux" ]; then
	# generate Unix Makefiles
	mkdir -p chibi-build/makefiles
	cd chibi-build/makefiles && cmake -G "Unix Makefiles" ../cmake-files
	cd "$root"
fi

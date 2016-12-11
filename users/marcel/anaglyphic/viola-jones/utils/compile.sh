#!/bin/sh

g++ -std=c++0x -lpng -O3 extractnw.cpp -o extractnw `libpng-config --ldflags`
g++ -std=c++0x -lpng -O3 nwtopng.cpp -o nwtopng `libpng-config --ldflags`
chmod +x extractnw nwtopng

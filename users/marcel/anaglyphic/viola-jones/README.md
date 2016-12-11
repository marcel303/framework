Viola and Jones Object Detection Framework (C++)
=========
<br/>
This is an implementation of the [Viola and Jones Object Detection Framework][1] in C++. It has been particularly optimized for the face detection paradigm:

  - Horizontal flipping face sample images in training phase.
  - Rotating negative sample images in training phase.
  - 4 Haar features implemented.
  - Integral image for faster processing.
  - Post-normalization speedup.
  - Scale invariant (feature scaling).

<br/>
Installation
--------------

```sh
git clone https://github.com/alexdemartos/ViolaAndJones.git violaandjones
cd violaandjones/src
g++ -std=c++0x -lpng -O3 Feature.cpp WeakClassifier.cpp StrongClassifier.cpp CascadeClassifier.cpp main.cpp -o vandj `libpng-config --ldflags`
chmod +x vandj
```

Usage
-----

  - Run `./vandj` without arguments to see the options.
  - No classification models are provided for object/face detection, you must train your own.
  - No images nor training samples are provided.

Example
-------

I've used this program to train a face detection algorithm. Here are some results:

<img src="http://www.alexdemartos.es/images/vandj_facedetect.jpg" width="500"/>

[1]:https://www.cs.cmu.edu/~efros/courses/LBMV07/Papers/viola-cvpr-01.pdf

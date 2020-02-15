[![Build Status](https://travis-ci.com/marcel303/framework.svg?branch=master)](https://travis-ci.com/marcel303/framework)

(Building a creative coding environment from scratch is a ton of work, so please consider becoming a patron!)
<br />[![Patreon](https://cloud.githubusercontent.com/assets/8225057/5990484/70413560-a9ab-11e4-8942-1a63607c0b00.png)](https://www.patreon.com/marcelsmit)

# README #

Hi! This is the origin of the Framework creative coding library.

## A creative coding library ##

Framework is a creative coding library developed in C++. It includes support for drawing images and playing sounds with one line of code, facilitating very rapid prototyping during creative code jams or workshops. It also includes easy access to keyboard, mouse and up to four gamepads. Shader programming is easy using the Shader class. All resources (images, sounds, shaders and more) can be tracked in real-time and hot-reloaded when changed.

In addition to the library itself, the code available here also includes a few side-projects built using Framework, including `audiograph` and `vfxgraph`. `audiograph` is a node based sound synthesis library and editor, while `vfxgraph` is intended for visuals and control. Both are built on top of `avgraph`, which provides a node based editor similar to VVVV and TouchDesigner.

## Design philosophy ##

The philosophy behind the design of Framework is that it should be fun to do coding!

As such, its language tries to be pragmatic and sparse, optimized for expression. The build system makes it easy to add apps and libraries and distribute packages. Code longevity is extended through a careful design of its vocabulary, slowly accumulating new functionality over time as it is needed. Build-times are kept low (usually a second or less) through careful monitoring and optimization of build times. Iteration is kept fast, through hot-reloading of resources when they are changed, and by delaying loading resources at startup until they are used (with full resource pre-loading being available for distributing builds).

## Features ##

- Multiple windows, keyboard, mouse and gamepads available everywhere. No callbacks needed,
- Hot-reloading of textures, models, shaders, or your own resources,
- Text drawing routines (SDF fonts or bitmapped),
- GX drawing api with backend implementations for OpenGL and Metal,
- GX immediate mode api. This api is similar in design to the legacy OpenGL interface for drawing primitives (gxBegin, gxEnd, gxVertex2f, etc). This api is intended for rapidly prototyping visuals,
- GX mesh api, using vertex and index buffers. This api allows for high-performance graphics, but is more rigid in nature than the immediate mode api,
- High quality primitive drawing, using signed distance fields for edge anti-aliasing,
- Surface class for easily drawing to textures, with built-in support for double buffering and post-processing,
- Render passes api with support for multiple render targets (mrt),
- Support for custom shader output semantics to aid development of shaders for use with mrt,
- GX capture api, for capturing GX immediate mode drawing into a GxMesh,
- Built-in shaders for hard and soft skinning, gaussian blurs, and more,
- Sprites and Spriter animations,
- FBX and glTF model loading and drawing,
- Stereo sound output, OGG/vorbis loading and streaming,
- Build system designed to make happy, not sad.

## Getting started ##

## What else is included? ##

### Framework libraries ###

- **avgraph** Reusable graph editor with real-time editing support,
- **audiograph** Graph-based system for real-time audio synthesis,
- **gltf** glTF model loading and drawing. Includes pbr shaders,
- **libreflection** Low-overhead reflection library,
- **libreflection-jsonio** json read-write for reflected types,
- **libreflection-textio** text read-write for reflected types,
- **fluidCube** 2d and 3d fluid dynamics (cpu and gpu),
- **libfbx** Speedy fbx parsing library,
- **libosc** OSC endpoints and threaded message handling, built on top of liboscpack,
- **libvideo** Threaded video decoding using libavcodec (ffmpeg),
- **libwebrequest** Perform web requests and download files using a download cache.

### Framework integration libraries ###

Framework includes a few 3rd party library integrations out of the box, including,

- **imgui-framework** Quickly add user interfaces to your projects using ImGui,
- **nanovg-framework** Draw antialiased paths using the gpu with NanoVG,
- **nanovg-canvas** Provides a canvas api on top of nanovg-framework,
- **jsusfx-framework** Provides Framework implementations for jsusfx's file and graphics apis, enabling jsusfx dsp effects to be run within the context of a Framework app,
- **jsusfx-audiograph** jsusfx audio nodes for audiograph,
- **jgmod-audiograph** jgmod audio nodes for audiograph.

### Third party libraries ###

This is a list of libraries made readily available through this repository.

- **box2d** Add real-time 2d physics to your apps,
- **libetherdream** Control laser projectors using the Etherdream controller,
- **leapmotion** Track hands and fingers using the Leap Motion controller,
- **libfreenect2** (macOS only) Receive depth and color images from Kinect v2 sensors,
- **PS3EYEDriver** Connect PS3 eye cameras through usb and receive camera and microphone streams into your apps,
- **Syphon** (macOS only) Share your visuals with other applications,
- **DeepBelief** (macOS only) Use machine learning to classify images,
- **rapidjson** Parse and write json documents,
- **tinyxml2** Parse and write xml documents,
- **portaudio** Multichannel audio input and output,
- **rtmidi** Cross-platform midi input and output,
- **nfd** Native file dialog library,
- **oscpack** Send and receive OSC messages, to let your app communicate with the outside world,
- **jsusfx** Jesusonic scripting language and framework for creating dsp plugins for the Reaper digital audio workstation (daw),
- **FreeImage** Load and save most image formats,
- **freetype2** Load and render glyphs for many font formats,
- **libsdl2** Create SDL2 applications (or let Framework manage things for you),
- **msdfgen** Create MSDF signed distance fields from paths,
- **oggvorbis** Read and write ogg/vorbis files,
- **turbojpeg** Fast jpeg encoding and decoding library,
- **libjgmod** A relic from the past! Plays xm, mod, s3m and (many) it files,
- **utf8rewind** Full-featured utf8 library,
- **xmm** Use various classifiers to identify and track gestures.



## `vfxgraph` Node based visuals and control ##

* `vfxgraph` is a visual node based system, adopting the data-flow paradigm to programming.
* `vfxgraph` makes it easy to add new node types by either hand-crafting a types.xml file, or by defining them in code.
* The environment allows one to inspect and preview any input or output socket. Persistent socket value visualizers can be added by right clicking on a socket.
* Graph changes are applied in real-time and previewed behind the editor, providing immediate visual feedback of the work you are doing.

![avGraph](/vfxGraph/docs/avGraph2.png)

## `audiograph` Sound synthesis and performance demo ##

* `4dworld` is a sound synthesis and performance tool, using the graph editor included with `avgraph` to modularly design sounds and effects.
* `4dworld` includes an audio synthesis backend which makes it easy to experiment with sound design in real-time. It extends real-time editing by propagating changes to all instances of a graph.
* This demo app shows how to create multiple instances from one graph. Each bird and machine uses its own `audiograph` instance for its sound synthesis.

## Contacting the author ##

For questions or suggestions, send me an e-mail at marcel303 [at] gmail.com, or reach out to me on Github.

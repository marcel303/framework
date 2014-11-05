#pragma once

// Collision Detection
namespace CD
{
	class Contact;
	class Cube;
	class GroupFlt;
	class GroupFltACL;
	class GroupFltEX;
	class Object;
	class Plane;
	class Scene;
}

// Physics
namespace Phy
{
	class Object;
	class Scene;
}

// Math Extension
namespace Mx
{
	class Plane;
}

// Parse
namespace Parse
{
	class String;
	class Tokenizer;
}

// Engine
class ActionHandler;
class Archive;
class Client;
class Clock;
class ClockFactory;
class ClockWindows;
class Controller;
class Display;
class DisplaySDL;
class DisplayWindows;
class Engine;
class Entity;
class EntityLight;
class EntityLink;
class EntityParticles;
class EntitySound;
class Event;
class File;
class FileReader;
class FileSys;
class FileSysMgr;
class FileSysNative;
class Frustum;
class GraphicsDevice;
class GraphicsDeviceD3D;
class GraphicsDeviceGL;
class HandlePool;
class InputHandler;
class InputManager;
class Mat4x4;
class MatrixStack;
class Mesh;
class Particle;
class ParticleSrc;
class ParticleSrcCone;
class ParticleSrcMod;
class ParticleSys;
class ProcTexcoord;
class ProcTexcoordMatrix;
class ProcTexcoordMatrix2D;
class ProcTexcoordMatrix2DAutoAlign;
class Renderer;
class RenderList;
class Scene;
class SceneRenderer;
class ShaderParam;
class ShapeBuilder;
class SoundDevice;
class System;
class Timer;
class Vec2;
class Vec3;
class Vec4;

// Engine: OpenGL


// Engine: D3D
class D3DResult;

// Engine: Network
class Address;
class Channel;
class ChannelHandler;
class ChannelManager;
class Packet;
class PacketDispatcher;
class PacketListener;

// Engine: Replication
namespace Replication
{
	class Client;
	class Handler;
	class Manager;
	class Object;
	class ObjectState;
}

// Engine: Resource
class Res;
class ResBaseTex;
class ResFont;
class ResIB;
class ResLoader;
class ResLoaderFont;
class ResLoaderPS;
class ResLoaderShader;
class ResLoaderSnd;
class ResLoaderTex;
class ResLoaderVS;
class ResMgr;
class ResPS;
class ResShader;
class ResSnd;
class ResSndSrc;
class ResTex;
class ResTexCD;
class ResTexCF;
class ResTexCR;
class ResTexD;
class ResTexR;
class ResTexRectR;
class ResTexV;
class ResUser;
class ResVB;
class ResVS;

// Game
class EntityPlayer;
class Game;
class Groups;
class PlayerControl;
class Weapon;

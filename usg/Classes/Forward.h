#pragma once

// forward declarations: core

class AnimTimer;
class Application;
class Archive;
class Atlas;
class Atlas_ImageInfo;
class AtlasImageMap;
class BinaryData;
class Camera;
class DeltaTimer;
class LogicTimer;
class PolledTimer;
class FontMap;
class Res;
class SelectionBuffer;
class SelectionMap;
class SoundEffect;
class SpriteGfx;
class Stream;
class TextureAtlas;
class TexturePVR;
class TextureRGBA;
class Timer;
class TimeTracker;
class VectorShape;

// forward declarations: core (platform specific)

class DisplaySDL;
class TouchMgr_Win32;

#ifndef OpenALState
class OpenALState;
#endif

// forward declarations: game

namespace Game
{
	class Entity;
	class EntityEnemy;
	class EntityPlayer;
	class GameHelp;
	class GameNotification;
	class GameRound;
	class GameSave;
	class GameScore;
	class GameSettings;
	class Grid_Effect;
	class HelpState;
	class IPlayerController;
	class UsgSocialListener;
	class SpriteEffectMgr;
	class VirtualInput;
	class VirtualKeyBoard;
	class World;
	class WorldBorder;
	
	class View_BanditIntro;
	class View_Credits;
	class View_CustomSettings;
	class View_GameOver;
	class View_GameSelect;
	class View_KeyBoard;
	class View_Main;
	class View_Options;
	class View_Pause;
	class View_PauseTouch;
	class View_ScoreEntry;
	class View_Scores;
	class View_ScoresPSP;
	class View_ScoreSubmit;
	class View_Upgrade;
	class View_UpgradeHD;
	class View_World;
};

namespace GameMenu
{
	class Button;
	class GuiCheckbox;
	class GuiListSlider;
	class GuiSlider;
	class MenuMgr;
};

// forward declarations: bandits

namespace Bandits
{
	class EntityBandit;
};

// forward declarations: GRS

namespace GRS
{
	class HttpClient;
	class QueryResult;
};

// forward declarations: extern!

class Atlas_ImageInfo;

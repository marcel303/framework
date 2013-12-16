# todo: read content list from text files

include ../Makefile.defs

SRC_DIR = content/content/
DST_DIR = Resources/

# libraries

LIBGG = ../libgg/libgg.a
#LIGIPHONE = ../libiphone/libiphone.a
LIBRES = ../tools/rescompiler/libres.a

# tools

TOOL_RESCOMPILE = ../tools/rescompiler/rescompiler

# game

#	Classes/Debugging_USG.cpp \

GAME = critwave
GAME_SRC = \
	Classes/Archive.cpp \
	Classes/BinaryData.cpp \
	Classes/BoundingBox2.cpp \
	Classes/BoundingSphere2.cpp \
	Classes/Camera.cpp \
	Classes/DirtyMgr.cpp \
	Classes/Display_Sdl.cpp \
	Classes/Fader.cpp \
	Classes/FontMap.cpp \
	Classes/GameState.cpp \
	Classes/GameView_Win32.cpp \
	Classes/Graphics.cpp \
	Classes/Graphics_OpenGL.cpp \
	Classes/GraphicsI.cpp \
	Classes/GuiButton.cpp \
	Classes/GuiCheckbox.cpp \
	Classes/GuiElement.cpp \
	Classes/GuiListSlider.cpp \
	Classes/GuiSlider.cpp \
	Classes/IView.cpp \
	Classes/LogicTimer.cpp \
	Classes/MaskMap.cpp \
	Classes/MD3.cpp \
	Classes/ParticleEffect.cpp \
	Classes/ParticleGenerator.cpp \
	Classes/PathCtx.cpp \
	Classes/PerfCount.cpp \
	Classes/Res.cpp \
	Classes/ResIO.cpp \
	Classes/ResMgr.cpp \
	Classes/Screenshot.cpp \
	Classes/Selection.cpp \
	Classes/SelectionBuffer.cpp \
	Classes/SelectionId.cpp \
	Classes/SelectionMap.cpp \
	Classes/SelectionSBuffer.cpp \
	Classes/SpriteEffectMgr.cpp \
	Classes/Subversion.cpp \
	Classes/Svn.cpp \
	Classes/TextureAtlas.cpp \
	Classes/Textures.cpp \
	Classes/TriggerTimerEx.cpp \
	Classes/UsgResourcesMac.cpp \
	Classes/UsgTexturesMac.cpp \
	Classes/VectorComposition.cpp \
	Classes/VectorShape.cpp
GAME_SRC += \
	Classes/Game/AnalogController.cpp \
	Classes/Game/Anim_ScreenFlash.cpp \
	Classes/Game/AnimSeq.cpp \
	Classes/Game/Bandit.cpp \
	Classes/Game/BanditEntity.cpp \
	Classes/Game/BanditIO.cpp \
	Classes/Game/BanditSeq.cpp \
	Classes/Game/BossBase.cpp \
	Classes/Game/BossFactory.cpp \
	Classes/Game/Boss_Magnet.cpp \
	Classes/Game/Boss_Snake.cpp \
	Classes/Game/Boss_Spinner.cpp \
	Classes/Game/DebrisMgr.cpp \
	Classes/Game/EncouragementTexts.cpp \
	Classes/Game/EnemyWaveMgr.cpp \
	Classes/Game/Entity.cpp \
	Classes/Game/EntityBullet.cpp \
	Classes/Game/EntityDebris.cpp \
	Classes/Game/EntityEnemy.cpp \
	Classes/Game/EntityPlayer.cpp \
	Classes/Game/EntityPool.cpp \
	Classes/Game/EntityPowerup.cpp \
	Classes/Game/EntitySpawn.cpp \
	Classes/Game/EventHandler_PlayerController_Gamepad.cpp \
	Classes/Game/EventHandler_PlayerController_Keyboard.cpp \
	Classes/Game/FireController.cpp \
	Classes/Game/ForceField.cpp \
	Classes/Game/GameHelp.cpp \
	Classes/Game/GameHighScore.cpp \
	Classes/Game/GameNotification.cpp \
	Classes/Game/GameRound.cpp \
	Classes/Game/GameSave.cpp \
	Classes/Game/GameScore.cpp \
	Classes/Game/GameSettings.cpp \
	Classes/Game/Gauge.cpp \
	Classes/Game/Grid_Effect.cpp \
	Classes/Game/GrsIntegration.cpp \
	Classes/Game/Health_Segmented.cpp \
	Classes/Game/IntermezzoMgr.cpp \
	Classes/Game/InvincibilityTimer.cpp \
	Classes/Game/IPlayerController.cpp \
	Classes/Game/LaserBeam.cpp \
	Classes/Game/LaserBeamMgr.cpp \
	Classes/Game/LimitedPulseTimer.cpp \
	Classes/Game/MaxiLaserBeam.cpp \
	Classes/Game/Menu.cpp \
	Classes/Game/Menu_BanditAnnounce.cpp \
	Classes/Game/Menu_Cheats.cpp \
	Classes/Game/Menu_Credits.cpp \
	Classes/Game/Menu_CustomSettings.cpp \
	Classes/Game/Menu_Empty.cpp \
	Classes/Game/Menu_GamepadSetup.cpp \
	Classes/Game/Menu_GameSelect.cpp \
	Classes/Game/Menu_InGame.cpp \
	Classes/Game/Menu_Main.cpp \
	Classes/Game/MenuMgr.cpp \
	Classes/Game/Menu_Options.cpp \
	Classes/Game/Menu_OptionsAdvanced.cpp \
	Classes/Game/Menu_Paused.cpp \
	Classes/Game/MenuRender.cpp \
	Classes/Game/Menu_ScoreEntry.cpp \
	Classes/Game/Menu_Scores.cpp \
	Classes/Game/Menu_ScoreSubmit.cpp \
	Classes/Game/Menu_Upgrade.cpp \
	Classes/Game/Menu_UpgradeBought.cpp \
	Classes/Game/MiniEntityMgr.cpp \
	Classes/Game/MissileTrail.cpp \
	Classes/Game/PlayerController.cpp \
	Classes/Game/PlayerController_AutoAim.cpp \
	Classes/Game/PlayerController_Gamepad.cpp \
	Classes/Game/PlayerController_Keyboard.cpp \
	Classes/Game/PlayerWeapons.cpp \
	Classes/Game/PowerSink.cpp \
	Classes/Game/RandomPicker.cpp \
	Classes/Game/Reward.cpp \
	Classes/Game/SceneMgt.cpp \
	Classes/Game/ScoreBoard.cpp \
	Classes/Game/ScoreEffect.cpp \
	Classes/Game/ScreenLock.cpp \
	Classes/Game/SectorGrid.cpp \
	Classes/Game/SocialIntegration.cpp \
	Classes/Game/SpecialController.cpp \
	Classes/Game/SpriteAnim.cpp \
	Classes/Game/StartMenu.cpp \
	Classes/Game/TargetingController.cpp \
	Classes/Game/TempRender.cpp \
	Classes/Game/TouchZoomController.cpp \
	Classes/Game/Traveller.cpp \
	Classes/Game/UiCriticalIndicator.cpp \
	Classes/Game/UiGrsRank.cpp \
	Classes/Game/Util_Follower.cpp \
	Classes/Game/Util_Path.cpp \
	Classes/Game/View_BanditIntro.cpp \
	Classes/Game/View_Credits.cpp \
	Classes/Game/View_CustomSettings.cpp \
	Classes/Game/View_GameOver.cpp \
	Classes/Game/View_GamepadSetup.cpp \
	Classes/Game/View_GameSelect.cpp \
	Classes/Game/View_KeyBoard.cpp \
	Classes/Game/View_Main.cpp \
	Classes/Game/View_Options.cpp \
	Classes/Game/View_Pause.cpp \
	Classes/Game/View_PauseTouch.cpp \
	Classes/Game/View_ScoreEntry.cpp \
	Classes/Game/View_Scores.cpp \
	Classes/Game/View_ScoreSubmit.cpp \
	Classes/Game/View_Upgrade.cpp \
	Classes/Game/View_UpgradeHD.cpp \
	Classes/Game/View_World.cpp \
	Classes/Game/VirtualKeyboard.cpp \
	Classes/Game/World.cpp \
	Classes/Game/WorldBorder.cpp \
	Classes/Game/WorldGrid.cpp
GAME_SRC += \
	../prototypes/vecrend/CompiledShape.cpp \
	../tools/atlcompiler/Atlas.cpp \
	../tools/fontcompiler/CompiledFont.cpp \
	../tools/rescompiler/ResIndex.cpp \
	../tools/veccompiler/CompiledComposition.cpp \
	../tools/pkgcompiler/CompiledPackage.cpp
GAME_SRC += \
	../libgrs/libgrs_cpp/grs.cpp \
	../libgrs/libgrs_cpp/grs_exchange.cpp \
	../libgrs/libgrs_cpp/grs_protocol.cpp \
	../libgrs/libgrs_cpp/grs_types.cpp
GAME_SRC += \
	../libiphone/EventManager.cpp \
	../libiphone/Event_SDL.cpp \
	../libiphone/ISoundEffectMgr.cpp \
	../libiphone/ISoundEffectMgr_SFX.cpp \
	../libiphone/ISoundPlayer.cpp \
	../libiphone/OpenALState.cpp \
	../libiphone/OpenGLCompat.cpp \
	../libiphone/OpenGLUtil.cpp \
	../libiphone/SocialAPI.cpp \
	../libiphone/SocialAPI_Dummy.cpp \
	../libiphone/SoundEffect.cpp \
	../libiphone/SoundEffectMgr.cpp \
	../libiphone/SoundEffectMgr_OpenAL.cpp \
	../libiphone/SoundPlayer_OpenAL.cpp \
	../libiphone/SoundPlayerFactory.cpp \
	../libiphone/SpriteGfx.cpp \
	../libiphone/SpriteRender.cpp \
	../libiphone/SpriteRenderer_OpenGL.cpp \
	../libiphone/System.cpp \
	../libiphone/TouchDLG.cpp \
	../libiphone/TouchInfo.cpp \
	../libiphone/TouchMgr_Win32.cpp \
	../libiphone/Util_ColorEx.cpp
# OGG
GAME_SRC += \
	./3rdparty/libogg-1.3.0/src/bitwise.c \
	./3rdparty/libogg-1.3.0/src/framing.c
# Vorbis
GAME_SRC += \
	./3rdparty/libvorbis-1.3.2/lib/analysis.c \
	./3rdparty/libvorbis-1.3.2/lib/bitrate.c \
	./3rdparty/libvorbis-1.3.2/lib/block.c \
	./3rdparty/libvorbis-1.3.2/lib/codebook.c \
	./3rdparty/libvorbis-1.3.2/lib/envelope.c \
	./3rdparty/libvorbis-1.3.2/lib/floor0.c \
	./3rdparty/libvorbis-1.3.2/lib/floor1.c \
	./3rdparty/libvorbis-1.3.2/lib/info.c \
	./3rdparty/libvorbis-1.3.2/lib/lookup.c \
	./3rdparty/libvorbis-1.3.2/lib/lpc.c \
	./3rdparty/libvorbis-1.3.2/lib/lsp.c \
	./3rdparty/libvorbis-1.3.2/lib/mapping0.c \
	./3rdparty/libvorbis-1.3.2/lib/mdct.c \
	./3rdparty/libvorbis-1.3.2/lib/psy.c \
	./3rdparty/libvorbis-1.3.2/lib/registry.c \
	./3rdparty/libvorbis-1.3.2/lib/res0.c \
	./3rdparty/libvorbis-1.3.2/lib/sharedbook.c \
	./3rdparty/libvorbis-1.3.2/lib/smallft.c \
	./3rdparty/libvorbis-1.3.2/lib/synthesis.c \
	./3rdparty/libvorbis-1.3.2/lib/vorbisfile.c \
	./3rdparty/libvorbis-1.3.2/lib/window.c \

GAME_SRC += Classes/ConfigState.cpp

ifeq ($(OS), linux)
	GAME_SRC += ../libiphone/System_Linux.cpp
endif
ifeq ($(OS), macos)
	GAME_SRC += ../libiphone/AudioOutput.cpp
	GAME_SRC += ../libiphone/AudioStreamVorbis.cpp
	GAME_SRC += ../libiphone/System_Linux.cpp
endif
ifeq ($(OS), win)
	GAME_SRC += ../libiphone/System_Win32.cpp
endif

ifndef $(PF)
	PF = ios
endif
	
#GAME_DST = ($(GAME_SRC:.cpp=.o):.mm=.o)
#GAME_DST = $(GAME_SRC:.cpp=.o)
GAME_DST = $(patsubst %.c,%.o,$(patsubst %.cpp,%.o,$(GAME_SRC)))
#GAME_DST = $(patsubst %.c,%.o,$(GAME_SRC))

GAME_INCLUDE = -I../libgg -I../libiphone -IClasses -IClasses/Game -I../libgrs/libgrs_cpp -I../tools/rescompiler -I../prototypes/vecrend -I../tools/fontcompiler -I../tools/atlcompiler -I../tools/veccompiler -I../tools/pkgcompiler -I./3rdparty/libogg-1.3.0/include -I./3rdparty/libvorbis-1.3.2/include

CFLAGS += -DHELP_VOICEOVER=1 -DUSE_MENU_SELECT=1 -DENABLE_RESOURCE_OVERRIDES=1

#all : content

all :
	@echo NOP

content : makebuild
	./makebuild$(EXECUTABLE_EXT) -c resources.txt -o build.txt -a textures.atl -as 512 -p $(PF)
	$(MAKE) -f build.txt all

content_rebuild : makebuild
	./makebuild$(EXECUTABLE_EXT) -f -c resources.txt -o build.txt -a textures.atl -as 512 -p $(PF)
	$(MAKE) -f build.txt all

$(LIBGG) : dummy
	cd ../libgg && $(MAKE)

#$(LIBIPHONE) : dummy
#	cd ../libiphone && $(MAKE)

game : $(GAME)

makebuild : makebuild.o dummy
	gcc -o makebuild$(EXECUTABLE_EXT) makebuild.o $(LIBRES) $(LIBGG) -lstdc++

clean :
	-rm $(GAME_DST)
	-rm $(GAME)

$(GAME) : $(GAME_DST) $(LIBGG) $(LIBIPHONE)
ifeq ($(OS), macos)
	gcc -o $(GAME) $(GAME_DST) $(LIBIPHONE) $(LIBGG) `sdl-config --static-libs` -lFreeImage -framework OpenAL -framework OpenGL -framework AppKit -lstdc++
else
ifeq ($(OS), linux)
	gcc -o $(GAME) $(GAME_DST) $(LIBIPHONE) $(LIBGG) -lSDL -lSDLmain -lFreeImage -lopenal -lGL -lstdc++
else
	gcc -o $(GAME) $(GAME_DST) $(LIBIPHONE) $(LIBGG) -lmingw32 -lSDLmain -lSDL -lFreeImage -lOpenAL32 -lOpenGL32 -lstdc++
endif
endif

%.o : %.cpp
	gcc -c $< -o $@ $(CFLAGS) $(GAME_INCLUDE)
# -I../libgg -I../libiphone -IClasses -IClasses/Game -I../libgrs/libgrs_cpp -I../tools/rescompiler -I../prototypes/vecrend -I../tools/fontcompiler -I../tools/atlcompiler -I../tools/veccompiler -I../tools/pkgcompiler

%.o : %.c
	gcc -c $< -o $@ $(CFLAGS) $(GAME_INCLUDE)

%.o : %.mm
	gcc -x c++ -c $< -o $@ $(CFLAGS) $(GAME_INCLUDE)

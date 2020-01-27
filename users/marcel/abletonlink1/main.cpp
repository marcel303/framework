#include "framework.h"
#include "abletonLink.h"

int main(int argc, char * argv[])
{
	setupPaths(CHIBI_RESOURCE_PATHS);
	
	if (!framework.init(300, 140))
		return -1;
	
	AbletonLink link;
	link.init(120.f);
	
	link.enable(true);
	link.enableStartStopSync(true);
	
	const int buttonX = 100;
	const int buttonY = 40;
	const int buttonSx = 30;
	const int buttonSy = 20;
	
	for (;;)
	{
		framework.process();
		
		if (framework.quitRequested)
			break;
		
		// input
		
		{
			auto sessionState = link.captureAppSessionState();
			
			bool sessionStateIsDirty = false;
			
			if (keyboard.wentDown(SDLK_SPACE))
			{
				sessionStateIsDirty = true;
				sessionState.setIsPlayingAtTick(!sessionState.isPlaying(), link.getClockTick());
			}
			
			if (keyboard.wentDown(SDLK_t))
			{
				sessionStateIsDirty = true;
				sessionState.setTempoAtTick(random<float>(60.f, 180.f), link.getClockTick());
			}
			
			if (sessionStateIsDirty)
			{
				sessionStateIsDirty = false;
				sessionState.commitApp();
			}
		}
		
		{
			const bool isInside =
				mouse.x >= buttonX && mouse.x < buttonX + buttonSx &&
				mouse.y >= buttonY && mouse.y < buttonY + buttonSy;
			
			if (isInside && mouse.wentDown(BUTTON_LEFT))
			{
				link.enable(!link.isEnabled());
			}
		}
		
		// display
		
		auto sessionState = link.captureAppSessionState();
		
		const auto tick = link.getClockTick();
		
		const double quantum = 4.0;
		
		double beat = sessionState.beatAtTick(tick, quantum);
		double phase = sessionState.phaseAtTick(tick, quantum);
		
		framework.beginDraw(100, 100, 100, 255);
		{
			setFont("calibri.ttf");
			pushFontMode(FONT_SDF);
			
			setColor(colorWhite);
			drawText(10, 10, 14, +1, +1, "beat: %.2f", beat);
			drawText(10, 30, 14, +1, +1, "phase: %.2f", phase);
			drawText(10, 50, 14, +1, +1, "tempo: %.2f", sessionState.tempo());
			
			// draw beat/phase progress bar
			
			hqBegin(HQ_FILLED_ROUNDED_RECTS);
			{
				setColor(80, 80, 80);
				hqFillRoundedRect(10, 100, 210, 110, 4.f);
				
				setColor(sessionState.isPlaying() ? colorGreen : colorRed);
				hqFillRoundedRect(10, 100, 10 + 200 * phase / quantum, 110, 4.f);
			}
			hqEnd();
			
			// draw 'link' button
			
			{
				hqBegin(HQ_FILLED_ROUNDED_RECTS);
				{
					setColor(link.isEnabled() ? Color(0, 255, 0) : Color(200, 200, 200));
					hqFillRoundedRect(buttonX, buttonY, buttonX + buttonSx, buttonY + buttonSy, 4.f);
				}
				hqEnd();
				
				setColor(colorBlack);
				drawText(buttonX + buttonSx/2, buttonY + buttonSy/2, 12, 0, 0, "Link");
			}
			
			popFontMode();
		}
		framework.endDraw();
	}
	
	link.shut();
	
	framework.shutdown();
	
	return 0;
}

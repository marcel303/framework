#include "framework-allegro2.h"
#include "jgmod.h"
#include "jgvis.h"

#include "allegro2-timerApi.h"
#include "allegro2-voiceApi.h"

#include "fileEditor_jgmod.h"
#include "framework.h"
#include "imgui.h"

FileEditor_Jgmod::FileEditor_Jgmod(const char * path)
	: timerApi(AllegroTimerApi::kMode_Manual)
	, voiceApi(192000, false)
	, voiceMixer(&voiceApi, &timerApi)
{
	jgmod = jgmod_load(path);
	
	player.init(64, &timerApi, &voiceApi);
	player.play(jgmod, true);
	
	audioOutput.Initialize(2, 192000, 32);
	audioOutput.Play(&voiceMixer);
}

FileEditor_Jgmod::~FileEditor_Jgmod()
{
	audioOutput.Shutdown();
	
	jgmod_destroy(jgmod);
}

bool FileEditor_Jgmod::wantsTick(const bool hasFocus, const bool inputIsCaptured)
{
	if (hasFocus == false)
		return false;
	
	return true;
}

void FileEditor_Jgmod::doButtonBar()
{
	if (ImGui::Button("Help"))
	{
		ImGui::OpenPopup("Help");
	}
	
	if (ImGui::BeginPopupModal("Help"))
	{
		ImGui::Text("Controls:");
		
		ImGui::Indent();
		ImGui::PushStyleColor(ImGuiCol_Text, (ImU32)ImColor(255, 255, 0));
		
		ImGui::Text("Left: Go to previous track");
		ImGui::Text("Right: Go to next track");
		
		ImGui::Text("+: Increase volume");
		ImGui::Text("-: Decrease volume");
		
		ImGui::Text("F1: Decrease playback speed");
		ImGui::Text("F2: Increase playback speed");
		ImGui::Text("F3: Decrease pitch");
		ImGui::Text("F4: Increase pitch");
		
		ImGui::Text("F5: Increase visual note length");
		ImGui::Text("F6: Decrease visual note length");
		
		ImGui::Text("F7: Decrease visual note offset");
		ImGui::Text("F8: Increase visual note offset");
		
		ImGui::Text("R: Restart playback");
		ImGui::Text("P or SPACE: Pause/resume playback");
		
		ImGui::Text("UP/DOWN: Scroll through the channel list");
		
		ImGui::Unindent();
		ImGui::PopStyleColor();

		if (ImGui::Button("Close"))
			ImGui::CloseCurrentPopup();
		
		ImGui::EndPopup();
	}
}

void FileEditor_Jgmod::tick(const int sx, const int sy, const float dt, const bool hasFocus, bool & inputIsCaptured)
{
	jgvis_tick(vis, player, inputIsCaptured);
	
	//
	
	clearSurface(0, 0, 0, 0);
	
	setFont("unispace.ttf");
	pushFontMode(FONT_SDF);
	{
		gxTranslatef(12, 12, 0);
		jgvis_draw(vis, player, true);
	}
	popFontMode();
}

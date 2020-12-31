#include "imgui/colorGradingLutDesigner.h"

#include "imgui-framework.h"

int main(int argc, char * argv[])
{
	setupPaths(CHIBI_RESOURCE_PATHS);
	
	framework.filedrop = true;
	
	if (!framework.init(800, 600))
		return -1;
	
	FrameworkImGuiContext guiContext;
	guiContext.init();
	
	ImGui::ColorGradingEditor editor;
	
	for (;;)
	{
		framework.process();
		
		if (framework.quitRequested)
			break;
		
		for (auto & file : framework.droppedFiles)
			strcpy(editor.previewFilename, file.c_str());
			
		bool inputIsCaptured = false;
		
		guiContext.processBegin(framework.timeStep, 800, 600, inputIsCaptured);
		{
			if (ImGui::Begin("Window", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
			{
				editor.Edit();
			}
			ImGui::End();
		}
		guiContext.processEnd();
		
		framework.beginDraw(0, 0, 0, 0);
		{
			editor.DrawPreview();
			
			if (editor.previewFilename[0] == 0)
			{
				drawText(400, 300, 24, 0, 0, "Drag an image here for a preview");
			}
			
			guiContext.draw();
		}
		framework.endDraw();
	}
	
	guiContext.shut();
	
	framework.shutdown();
	
	return 0;
}

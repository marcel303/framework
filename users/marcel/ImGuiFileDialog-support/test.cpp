#include "framework.h"
#include "imgui-framework.h"

#include "ImGuiFileDialog.h"

int main(int argc, char * argv[])
{
	setupPaths(CHIBI_RESOURCE_PATHS);

	if (!framework.init(800, 600))
		return -1;
	
	FrameworkImGuiContext guiContext;
	guiContext.init();
	
	for (;;)
	{
		framework.process();
		
		if (framework.quitRequested)
			break;
		
		bool inputIsCaptured = false;
		guiContext.processBegin(framework.timeStep, 800, 600, inputIsCaptured);
		{
			if (ImGui::Begin("Window"))
			{
				if (ImGui::Button("Open File Dialog"))
				{
					const char * filters = "{.txt,.md},Source files (*.cpp *.h *.hpp){.cpp,.h,.hpp},Image files (*.png *.gif *.jpg *.jpeg){.png,.gif,.jpg,.jpeg}";
					
					ImGuiFileDialog::Instance()->OpenModal(
						"FileDialog",
						"File Dialog",
						filters,
						getDirectory(),
						"");
				}
			}
			ImGui::End();
			
			const ImVec2 maxSize = ImVec2(800, 600);
			const ImVec2 minSize = ImVec2(400, 300);

			if (ImGuiFileDialog::Instance()->Display("FileDialog", ImGuiWindowFlags_NoCollapse, minSize, maxSize))
			{
				if (ImGuiFileDialog::Instance()->IsOk())
				{
					logInfo("User chose a file!");
				}
				
				ImGuiFileDialog::Instance()->Close();
			}
		}
		guiContext.processEnd();
		
		framework.beginDraw(0, 0, 0, 0);
		{
			guiContext.draw();
		}
		framework.endDraw();
	}
	
	guiContext.shut();

	framework.shutdown();

	return 0;
}

#include "framework.h"

// jsfx
#include "jsusfx.h"
#include "jsusfx_file.h"
#include "jsusfx_gfx.h"
#include "jsusfx_serialize.h"

// jsfx framework
#include "gfx-framework.h"
#include "jsusfx-framework.h"

// text editor
#include "imgui-framework.h"
#include "imgui/TextEditor.h"
#include "TextIO.h"

// todo : add drag and drop support

int main(int argc, char * argv[])
{
#if defined(CHIBI_RESOURCE_PATH)
	changeDirectory(CHIBI_RESOURCE_PATH);
#endif
	
	//framework.allowHighDpi = false;
	
	if (!framework.init(1200, 800))
		return -1;
	
	FrameworkImGuiContext guiContext;
	guiContext.init();
	
	TextEditor textEditor;
	textEditor.SetLanguageDefinition(TextEditor::LanguageDefinition::CPlusPlus());
	
	std::vector<std::string> lines;
	TextIO::LineEndings lineEndings;
	TextIO::load("test-edit.jsfx", lines, lineEndings);
	textEditor.SetTextLines(lines);
	
	// jsfx setup
	
	JsusFxPathLibrary_Basic path_library("");
	JsusFx_Framework jsfx(path_library);
	
	JsusFxFileAPI_Basic file_api;
	file_api.init(jsfx.m_vm);
	jsfx.fileAPI = &file_api;
	
	JsusFxGfx_Framework gfx_api(jsfx);
	gfx_api.init(jsfx.m_vm);
	jsfx.gfx = &gfx_api;
	
	bool textIsDirty = true;
	
	for (;;)
	{
		framework.process();
		
		if (framework.quitRequested)
			break;
		
		bool inputIsCaptured = false;
		guiContext.processBegin(framework.timeStep, framework.getMainWindow().getWidth(), framework.getMainWindow().getHeight(), inputIsCaptured);
		{
			const std::string oldText = textEditor.GetText();
			
			textEditor.Render("Text Editor");
			
			const std::string newText = textEditor.GetText();
			
			if (newText != oldText)
			{
				textIsDirty = true;
				
				std::vector<std::string> lines;
				TextIO::LineEndings lineEndings;
				TextIO::loadText(newText.c_str(), lines, lineEndings);
				TextIO::save("test-edit.jsfx", lines, lineEndings);
			}
		}
		guiContext.processEnd();
		
		if (textIsDirty)
		{
			textIsDirty = false;
			
			jsfx.compile(path_library, "test-edit.jsfx", JsusFx::kCompileFlag_CompileGraphicsSection);
			jsfx.prepare(44100, 256);
		}
		
		jsfx.process(nullptr, nullptr, 256, 0, 0);
		
		framework.beginDraw(255, 0, 0, 0);
		{
			setFont("calibri.ttf");
			
			//
			
			gfx_api.setup(nullptr, jsfx.gfx_w, jsfx.gfx_h, mouse.x, mouse.y, true);
			
			jsfx.draw();
			
			//
			
			pushSurface(nullptr);
			guiContext.draw();
			popSurface();
		}
		framework.endDraw();
	}
	
	guiContext.shut();
	
	framework.shutdown();
	
	return 0;
}

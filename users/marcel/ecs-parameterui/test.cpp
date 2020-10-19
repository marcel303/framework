#include "parameter.h"
#include "parameterUi.h"
#include "framework.h"
#include "imgui-framework.h"

/*

feature testing:

- all parameter types

- all parameter types, with limits

- color semantic for supported types

- parameter filter, with scroll into view, auto-expand

*/

int main(int argc, char * argv[])
{
	setupPaths(CHIBI_RESOURCE_PATHS);
	
	framework.init(800, 400);
	
	FrameworkImGuiContext guiCtx;
	guiCtx.init();
	
	ParameterMgr allParameterTypes;
	allParameterTypes.addBool("bool", false);
	allParameterTypes.addInt("int", 0);
	allParameterTypes.addFloat("float", 0.f);
	allParameterTypes.addVec2("vec2", Vec2());
	allParameterTypes.addVec3("vec3", Vec3());
	allParameterTypes.addVec4("vec4", Vec4());
	allParameterTypes.addString("string", "");
	allParameterTypes.addEnum("enum", 0, {{"a", 0}, {"b", 1}, {"c", 2}});
	
	ParameterMgr parameterTypesWithLimits;
	parameterTypesWithLimits.addInt("int", 0)->setLimits(-2, +8);
	parameterTypesWithLimits.addFloat("float", 0.f)->setLimits(-2.f, +8.f);
	parameterTypesWithLimits.addVec2("vec2", Vec2())->setLimits(Vec2(-2, -3), Vec2(8, 9));
	parameterTypesWithLimits.addVec3("vec3", Vec3())->setLimits(Vec3(-2, -3, -4), Vec3(8, 9, 10));
	parameterTypesWithLimits.addVec4("vec4", Vec4())->setLimits(Vec4(-2, -3, -4, -5), Vec4(8, 9, 10, 11));

	ParameterMgr parameterTypesWithColorSemantic;
	//parameterTypesWithLimits.addVec3("vec3", Vec3(), kParameterSemantic_Color);
	//parameterTypesWithLimits.addVec4("vec4", Vec4(), kParameterSemantic_Color);
	
	ParameterMgr parent;
	parent.init("parent");
	ParameterMgr childA;
	childA.init("A");
	childA.addInt("a", 0);
	childA.addInt("b", 1);
	childA.addInt("c", 2);
	ParameterMgr childB;
	childB.init("B");
	childB.addInt("x", 0);
	childB.addInt("y", 1);
	childB.addInt("z", 2);
	parent.addChildren(
		{
			&childA,
			&childB
		});
	
	char searchFilter[64] = {};
	
	for (;;)
	{
		framework.process();
		
		if (framework.quitRequested)
			break;
		
		bool inputIsCaptured = false;
		guiCtx.processBegin(framework.timeStep, 800, 400, inputIsCaptured);
		{
			ImGui::SetNextWindowPos(ImVec2(0, 0));
			ImGui::SetNextWindowSize(ImVec2(800, 400));
			if (ImGui::Begin("test", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize))
			{
				if (ImGui::CollapsingHeader("All Parameter Types"))
				{
					parameterUi::doParameterUi(allParameterTypes, nullptr, false);
				}
				
				if (ImGui::CollapsingHeader("Parameter Types With Limits"))
				{
					parameterUi::doParameterUi(parameterTypesWithLimits, nullptr, false);
				}
				
				if (ImGui::CollapsingHeader("Parameter Types With Color Semantic"))
				{
					parameterUi::doParameterUi(parameterTypesWithColorSemantic, nullptr, false);
				}
				
				if (ImGui::CollapsingHeader("Search Filter Behavior"))
				{
					ImGui::InputText("Search filter", searchFilter, sizeof(searchFilter) / sizeof(searchFilter[0]));
					if (ImGui::IsItemHovered())
					{
						ImGui::BeginTooltip();
						ImGui::Text("Expected behavior,");
						ImGui::Text("- Use original open/close state for tree nodes when the filter is empty.");
						ImGui::Text("- Only show items with one or more matches when filter is set.");
						ImGui::Text("- Automatically open tree nodes with one or more matches when filter is set.");
						ImGui::EndTooltip();
					}
				
					parameterUi::doParameterUi_recursive(parent, searchFilter);
				}
			}
			ImGui::End();
		}
		guiCtx.processEnd();
		
		framework.beginDraw(0, 0, 0, 0);
		{
			guiCtx.draw();
		}
		framework.endDraw();
	}
	
	guiCtx.shut();
	
	framework.shutdown();
	
	return 0;
}


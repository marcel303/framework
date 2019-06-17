// max objects includes
#include <string>
#include <vector>

// UI app includes
#include "controlSurfaceDefinition.h"
#include "controlSurfaceDefinitionEditing.h"
#include "framework.h"
#include "osc/OscOutboundPacketStream.h"
#include "oscSender.h"
#include "reflection-bindtofile.h"

// Max/MSP generator app includes
#include "framework.h"
#include "maxPatchGenerator.h"
#include "reflection-jsonio.h"
#include "StringEx.h"
#include <stdio.h>
#include <unistd.h> // fixme

// ++++ : move UI generator to a separate source file
// ++++ : refine UI generator
// ++++ : add reflection type UI structure

// ++++ : move Max/MSP patch generator to its own source file
// todo : determine a way to include Max/MSP patch snippets, so complicated stuff can be generated externally

// todo : create UI app, which reads reflected UI structure from file and allows knob control, OSC output and creating presets

//

int main(int arg, char * argv[])
{
	chdir(CHIBI_RESOURCE_PATH);
	
	TypeDB typeDB;
	
	typeDB.addPlain<bool>("bool", kDataType_Bool);
	typeDB.addPlain<int>("int", kDataType_Int);
	typeDB.addPlain<float>("float", kDataType_Float);
	typeDB.addPlain<std::string>("string", kDataType_String);
	
	max::AppVersion::reflect(typeDB);
	max::SavedAttributeAttributes::reflect(typeDB);
	max::Box::reflect(typeDB);
	max::Line::reflect(typeDB);
	max::PatchBox::reflect(typeDB);
	max::PatchLine::reflect(typeDB);
	max::Patcher::reflect(typeDB);
	max::Patch::reflect(typeDB);
	
	ControlSurfaceDefinition::reflect(typeDB);
	
	//
	
#if 1
	//{
		ControlSurfaceDefinition::Surface surface;
		
		ControlSurfaceDefinition::SurfaceEditor surfaceEditor(&surface);
	
		surfaceEditor
			.beginGroup("master")
				.beginLabel("master")
					.divideBottom()
					.size(400, 40)
					.end()
				.beginKnob("intensity")
					.defaultValue(5.f)
					.limits(0.f, 10.f)
					.exponential(2.f)
					.osc("/master/intensity")
					.end()
				.beginKnob("VU")
					.limits(0.f, 1.f)
					.exponential(2.f)
					.osc("/master/vu")
					.end()
				.separator()
				.beginKnob("A/B")
					.limits(0.f, 1.f)
					.exponential(2.f)
					.osc("/master/ab")
					.end()
				.beginListbox("mode")
					.item("a")
					.item("b")
					.defaultValue("b")
					.osc("/master/mode")
					.end()
				.endGroup();
		
		for (int i = 0; i < 7; ++i)
		{
			surfaceEditor
				.beginGroup("source")
					.beginLabel("source")
						.divideBottom()
						.end()
					.beginKnob("position")
						.limits(0.f, 1.f)
						.exponential(2.f)
						.end()
					.beginKnob("speed")
						.limits(0.f, 1.f)
						.exponential(2.f)
						.defaultValue(.3f)
						.end()
					.beginKnob("scale")
						.limits(0.f, 1.f)
						.exponential(2.f)
						.defaultValue(.2f)
						.end()
					.beginKnob("position")
						.limits(0.f, 1.f)
						.exponential(2.f)
						.end()
					.beginKnob("speed")
						.limits(0.f, 10.f)
						.exponential(2.f)
						.defaultValue(8.f)
						.end()
					.beginKnob("scale")
						.limits(0.f, 1.f)
						.exponential(2.f)
						.end()
					.endGroup();
		}
		
		surface.initializeDefaultValues();
		
		surfaceEditor.beginLayout()
			.size(800, 200)
			.margin(10, 10)
			.padding(4, 4)
			.end();
		
		saveObjectToFile(&typeDB, typeDB.findType(surface), &surface, "surface-definition.json");
		
		struct LiveUi
		{
			struct Elem
			{
				ControlSurfaceDefinition::Element * elem = nullptr;
				float value = 0.f;
				float defaultValue = 0.f;
				float doubleClickTimer = 0.f;
				bool valueHasChanged = false;
			};
			
			std::vector<Elem> elems;
			
			Elem * hoverElem = nullptr;
			
			Elem * activeElem = nullptr;
			
			std::vector<OscSender*> oscSenders;
			
			LiveUi & osc(const char * ipAddress, const int udpPort)
			{
				OscSender * sender = new OscSender();
				sender->init(ipAddress, udpPort);
				oscSenders.push_back(sender);
				return *this;
			}
			
			void addElem(ControlSurfaceDefinition::Element * elem)
			{
				elems.resize(elems.size() + 1);
				
				auto & e = elems.back();
				e.elem = elem;
				
				if (elem->type == ControlSurfaceDefinition::kElementType_Knob)
				{
					auto & knob = elem->knob;
					Assert(knob.hasDefaultValue);
					
					if (knob.hasDefaultValue)
					{
						const float t = (knob.defaultValue - knob.min) / (knob.max - knob.min);
						
						e.value = powf(t, 1.f / knob.exponential);
						e.defaultValue = e.value;
					}
				}
				else if (elem->type == ControlSurfaceDefinition::kElementType_Listbox)
				{
					auto & listbox = elem->listbox;
					Assert(listbox.hasDefaultValue);
					
					if (listbox.hasDefaultValue)
					{
						int index = 0;
						
						for (int i = 0; i < listbox.items.size(); ++i)
							if (listbox.items[i] == listbox.defaultValue)
								index = i;
						
						e.value = index;
						e.defaultValue = index;
					}
				}
			}
			
			void tick(const float dt)
			{
				hoverElem = nullptr;
				
				for (auto & e : elems)
				{
					auto * elem = e.elem;
					
					e.doubleClickTimer = fmaxf(0.f, e.doubleClickTimer - dt);
					
					const bool isInside =
						mouse.x >= elem->x &&
						mouse.x < elem->x + elem->sx &&
						mouse.y >= elem->y &&
						mouse.y < elem->y + elem->sy;
					
					if (isInside)
						hoverElem = &e;
					
					if (elem->type == ControlSurfaceDefinition::kElementType_Knob)
					{
						auto & knob = elem->knob;
						
						if (activeElem == nullptr && isInside && mouse.wentDown(BUTTON_LEFT))
						{
							activeElem = &e;
							SDL_CaptureMouse(SDL_TRUE);
							
							if (e.doubleClickTimer > 0.f)
							{
								if (knob.hasDefaultValue)
									e.value = e.defaultValue;
							}
							else
								e.doubleClickTimer = .2f;
						}
						
						if (&e == activeElem && mouse.wentUp(BUTTON_LEFT))
						{
							activeElem = nullptr;
							SDL_CaptureMouse(SDL_FALSE);
						}
						
						if (&e == activeElem)
						{
							const float speed = 1.f / (keyboard.isDown(SDLK_LSHIFT) ? 400.f : 100.f);
							
							const float oldValue = e.value;
							
							e.value = saturate<float>(e.value - mouse.dy * speed);
							
							if (e.value != oldValue)
							{
								e.valueHasChanged = true;
							}
						}
					}
					else if (elem->type == ControlSurfaceDefinition::kElementType_Listbox)
					{
						auto & listbox = elem->listbox;
						
						if (activeElem == nullptr && isInside && mouse.wentDown(BUTTON_LEFT))
						{
							activeElem = &e;
							SDL_CaptureMouse(SDL_TRUE);
							
							if (e.doubleClickTimer > 0.f)
							{
								if (listbox.hasDefaultValue)
									e.value = e.defaultValue;
							}
							else
								e.doubleClickTimer = .2f;
						}
						
						if (&e == activeElem && mouse.wentUp(BUTTON_LEFT))
						{
							activeElem = nullptr;
							SDL_CaptureMouse(SDL_FALSE);
						}
						
						if (&e == activeElem)
						{
							const float speed = 1.f / 100.f;
							
							const float oldValue = e.value;
							
							e.value = clamp<float>(e.value + mouse.dy * speed, 0.f, listbox.items.size());
							
							if (e.value != oldValue)
							{
							// todo: only changed when item index changes
								e.valueHasChanged = true;
							}
						}
					}
				}
				
				// send changed values over OSC
				
				char buffer[1200];
				osc::OutboundPacketStream s(buffer, 1200);
				int initialSize = 0;
				
				auto beginBundle = [&]()
				{
					s = osc::OutboundPacketStream(buffer, 1200);
					
					s << osc::BeginBundle();
					
					initialSize = s.Size();
				};
				
				auto sendBundle = [&]()
				{
					Assert(s.Size() != initialSize);
					
					try
					{
						s << osc::EndBundle;
						
						for (auto * oscSender : oscSenders)
							oscSender->send(s.Data(), s.Size());
					}
					catch (std::exception & e)
					{
						logError("%s", e.what());
					}
				};
				
				beginBundle();
				
				for (auto & e : elems)
				{
					if (e.valueHasChanged)
					{
						e.valueHasChanged = false;
						
						if (e.elem->type == ControlSurfaceDefinition::kElementType_Knob)
						{
							auto & knob = e.elem->knob;
							
							if (!knob.oscAddress.empty())
							{
								const float t = powf(activeElem->value, knob.exponential);
								const float value = knob.min * (1.f - t) + knob.max * t;
								
								if (s.Size() + knob.oscAddress.size() + 100 > 1200)
								{
									sendBundle();
									beginBundle();
								}
								
								s << osc::BeginMessage(knob.oscAddress.c_str());
								{
									s << value;
								}
								s << osc::EndMessage;
							}
						}
						else if (e.elem->type == ControlSurfaceDefinition::kElementType_Listbox)
						{
							auto & listbox = e.elem->listbox;
							
							if (!listbox.oscAddress.empty())
							{
								const int index = clamp<int>((int)floorf(e.value), 0, listbox.items.size() -1);
								
								if (s.Size() + listbox.oscAddress.size() + listbox.items[index].size() + 100 > 1200)
								{
									sendBundle();
									beginBundle();
								}
								
								s << osc::BeginMessage(listbox.oscAddress.c_str());
								{
									s << listbox.items[index].c_str();
								}
								s << osc::EndMessage;
							}
						}
					}
				}
				
				if (s.Size() != initialSize)
					sendBundle();
			}
			
			void draw() const
			{
				for (auto & e : elems)
				{
					auto * elem = e.elem;
					
					if (elem->type == ControlSurfaceDefinition::kElementType_Label)
					{
						auto & label = elem->label;
						
						setColor(40, 40, 40);
						drawText(elem->x, elem->y + elem->sy / 2.f, 12, +1, 0, "%s", label.text.c_str());
					}
					else if (elem->type == ControlSurfaceDefinition::kElementType_Knob)
					{
						hqBegin(HQ_FILLED_ROUNDED_RECTS);
						{
							if (&e == activeElem)
								setLumi(190);
							else if (&e == hoverElem)
								setLumi(210);
							else
								setLumi(200);
							hqFillRoundedRect(elem->x, elem->y, elem->x + elem->sx, elem->y + elem->sy, 4);
						}
						hqEnd();
						
						auto & knob = elem->knob;
						
						const float angle1 = (- 120 - 90) * float(M_PI) / 180.f;
						const float angle2 = (+ 120 - 90) * float(M_PI) / 180.f;
						
						const int numSteps = 100;
						
						const float radius = 14.f;
						
						const float midX = elem->x + elem->sx / 2.f;
						const float midY = elem->y + elem->sy / 2.f;
						
						hqBegin(HQ_LINES);
						{
							float x1;
							float y1;
							
							for (int i = 0; i < numSteps; ++i)
							{
								const float t = i / float(numSteps - 1);
								const float angle = angle1 * (1.f - t) + angle2 * t;
								
								float strokeSize;
								
								if (t < e.value)
								{
									setColor(200, 100, 100);
									strokeSize = 1.6f;
								}
								else
								{
									setColor(100, 100, 200);
									strokeSize = 1.f;
								}
								
								const float x2 = midX + cosf(angle) * radius;
								const float y2 = midY + sinf(angle) * radius;
								
								if (i != 0)
									hqLine(x1, y1, strokeSize, x2, y2, strokeSize);
								
								x1 = x2;
								y1 = y2;
							}
						}
						hqEnd();
						
						setColor(40, 40, 40);
						drawText(elem->x + elem->sx / 2.f, elem->y + elem->sy - 2, 10, 0, -1, "%s", knob.name.c_str());
					}
					else if (elem->type == ControlSurfaceDefinition::kElementType_Listbox)
					{
						hqBegin(HQ_FILLED_ROUNDED_RECTS);
						{
							if (&e == activeElem)
								setLumi(190);
							else if (&e == hoverElem)
								setLumi(210);
							else
								setLumi(200);
							hqFillRoundedRect(elem->x, elem->y, elem->x + elem->sx, elem->y + elem->sy, 4);
						}
						hqEnd();
						
						auto & listbox = elem->listbox;
						
						setColor(40, 40, 40);
						setFont("calibri.ttf");
						
						if (listbox.items.empty() == false)
						{
							const int index = clamp<int>((int)floorf(e.value),  0, listbox.items.size() - 1);
							drawText(elem->x + elem->sx / 2.f, elem->y + elem->sy / 2.f, 10, 0, 0, "%s", listbox.items[index].c_str());
						}
						
						const float midY = elem->y + elem->sy / 2.f;
						
						hqBegin(HQ_FILLED_TRIANGLES);
						{
							setLumi(100);
							hqFillTriangle(elem->x + 8, midY - 4, elem->x + 4, midY, elem->x + 8, midY + 4);
							
							setLumi(100);
							hqFillTriangle(elem->x + elem->sx - 8, midY - 4, elem->x + elem->sx - 4, midY, elem->x + elem->sx - 8, midY + 4);
						}
						hqEnd();
					}
					else if (elem->type == ControlSurfaceDefinition::kElementType_Separator)
					{
						hqBegin(HQ_FILLED_ROUNDED_RECTS);
						{
							setLumi(200);
							hqFillRoundedRect(elem->x, elem->y, elem->x + elem->sx, elem->y + elem->sy, 4);
						}
						hqEnd();
					}
				}
			}
			
			void drawTooltip() const
			{
				gxPushMatrix();
				{
					gxTranslatef(mouse.x, mouse.y, 0);
					
					if (activeElem != nullptr)
					{
						auto * elem = activeElem->elem;
						
						if (elem->type == ControlSurfaceDefinition::kElementType_Knob)
						{
							auto & knob = elem->knob;
							
							hqBegin(HQ_FILLED_ROUNDED_RECTS);
							{
								setColor(200, 200, 100);
								hqFillRoundedRect(0, 0, 100, 30, 4);
							}
							hqEnd();
							
							const float t = powf(activeElem->value, knob.exponential);
							const float value = knob.min * (1.f - t) + knob.max * t;
							
							setLumi(20);
							drawText(10, 10, 20, +1, +1, "%.2f", value);
						}
					}
				}
				gxPopMatrix();
			}
		};
		
		framework.init(800, 200);
		
		LiveUi liveUi;
		
		for (auto & group : surface.groups)
		{
			for (auto & elem : group.elems)
			{
				liveUi.addElem(&elem);
			}
		}
		
		liveUi
			.osc("127.0.0.1", 2000)
			.osc("127.0.0.1", 2002);
		
		for (;;)
		{
			framework.process();
			
			if (framework.quitRequested)
				break;
			
			liveUi.tick(framework.timeStep);
			
			const int c = 160;
			
			framework.beginDraw(c, c, c, 0);
			{
				setFont("calibri.ttf");
				
				liveUi.draw();
				
				liveUi.drawTooltip();
			}
			framework.endDraw();
		}
	//}
#endif

	//
	
	max::Patch patch;
	
	int nextObjectId = 1;
	
	auto allocObjectId = [&]() -> std::string
	{
		return String::FormatC("obj-%d", nextObjectId++);
	};
	
	auto connect = [&](const std::string & src, const int srcIndex, const std::string & dst, const int dstIndex)
	{
		max::PatchLine line;
		line.patchline.source = { src, String::FormatC("%d", srcIndex) };
		line.patchline.destination = { dst, String::FormatC("%d", dstIndex) };
		
		patch.patcher.lines.push_back(line);
	};
	
	max::PatchEditor patchEditor(patch);
	
#if 0
	std::string previousId;
	
	for (int i = 0; i < 100; ++i)
	{
		const std::string id = allocObjectId();
		patchEditor
			.beginBox(id.c_str(), 1, 1)
				.comment("My First Box")
				.maxclass("newobj")
				.patching_rect(10, i * 30, 40, 20)
				.presentation(true)
				.presentation_rect(10, i * 30, 40, 20)
				.text("gate")
				.end();
		
		if (!previousId.empty())
		{
			connect(previousId, 0, id, 0);
		}
		
		previousId = id;
	}
	
	for (int i = 0; i < 10; ++i)
	{
		const std::string osc_id = allocObjectId();
		patchEditor
			.beginBox(osc_id.c_str(), 1, 1)
				.patching_rect(100, 30 + i * 100, 40, 20)
				.text("4d.paramOsc /filterbankPlayerMidiChannel")
				.end();
		
		const std::string knob_id = allocObjectId();
		patchEditor
			.beginBox(knob_id.c_str(), 1, 2)
				.maxclass("live.dial")
				.patching_rect(100, 60 + i * 100, 40, 20)
				.presentation(true)
				.presentation_rect(100 + i * 40, 60, 40, 20)
				.parameter_enable(true)
				.varname("filterbankDecay")
				.saved_attribute_attributes(
					{
						{ "parameter_mmin", "1.0" },
						{ "parameter_mmax", "10.0" },
						{ "parameter_exponent", "2.0" },
						{ "parameter_longname", "filterbankDecay[1]" },
						{ "parameter_shortname",  "decay" },
						{ "parameter_type",  "0" },
						{ "parameter_unitstyle", "1" },
						{ "parameter_linknames", "1" }
					})
				.end();
		
		connect(osc_id, 0, knob_id, 0);
		connect(knob_id, 0, osc_id, 0);
	}
#else
	int patching_x = 10;
	int patching_y = 10;
	
	for (auto & group : surface.groups)
	{
		for (auto & elem : group.elems)
		{
			if (elem.type == ControlSurfaceDefinition::kElementType_Label)
			{
				const std::string id = allocObjectId();
				patchEditor
					.beginBox(id.c_str(), 1, 2)
						.maxclass("comment")
						.text(elem.label.text.c_str())
						.patching_rect(patching_x, patching_y, 40, 48) // live.dial has a fixed height of 48
						.presentation(true)
						.presentation_rect(elem.x, elem.y, elem.sx, elem.sy)
						.end();
				patching_y += 60;
			}
			else if (elem.type == ControlSurfaceDefinition::kElementType_Knob)
			{
			// todo : fill in the correct details
				const std::string knob_id = allocObjectId();
				patchEditor
					.beginBox(knob_id.c_str(), 1, 2)
						.maxclass("live.dial")
						.patching_rect(patching_x, patching_y, 40, 48) // live.dial has a fixed height of 48
						.presentation(true)
						.presentation_rect(elem.x, elem.y, elem.sx, elem.sy)
						.parameter_enable(true)
						.varname(elem.knob.name.c_str())
						.saved_attribute("parameter_mmin", elem.knob.min)
						.saved_attribute("parameter_mmax", elem.knob.max)
						.saved_attribute("parameter_initial_enable", 1)
						.saved_attribute("parameter_initial", elem.knob.defaultValue)
						.saved_attribute("parameter_exponent", elem.knob.exponential)
						.saved_attribute("parameter_longname", "filterbankDecay[1]")
						.saved_attribute("parameter_shortname",  elem.knob.name)
						.saved_attribute("parameter_type", 0)
						.saved_attribute("parameter_unitstyle", 1)
						.saved_attribute("parameter_linknames", 1)
						.end();
				patching_y += 60;
				
				if (elem.knob.oscAddress.empty() == false)
				{
					const std::string osc_id = allocObjectId();
					patchEditor
						.beginBox(osc_id.c_str(), 1, 1)
							.patching_rect(patching_x, patching_y, 40, 22) // automatic height will be 22 for 4d.paramOsc
							.text(String::FormatC("4d.paramOsc %s", elem.knob.oscAddress.c_str()).c_str())
							.end();
					patching_y += 60;
					
					connect(osc_id, 0, knob_id, 0);
					connect(knob_id, 0, osc_id, 0);
				}
			}
			else if (elem.type == ControlSurfaceDefinition::kElementType_Listbox)
			{
			// todo : fill in the correct details
				auto & listbox = elem.listbox;
				
				int defaultIndex = 0;
				for (int i = 0; i < listbox.items.size(); ++i)
					if (listbox.items[i] == listbox.defaultValue)
						defaultIndex = i;
				
				max::SavedAttribute attr("parameter_mmin", String::FormatC("%d", 0));
				
				const std::string listbox_id = allocObjectId();
				patchEditor
					.beginBox(listbox_id.c_str(), 1, 2)
						.maxclass("live.menu")
						.patching_rect(patching_x, patching_y, 40, 48) // live.dial has a fixed height of 48
						.presentation(true)
						.presentation_rect(elem.x, elem.y, elem.sx, elem.sy)
						.parameter_enable(true)
						.varname("filterbankDecay")
						.saved_attribute("parameter_mmin", 0)
						.saved_attribute("parameter_mmax", (int)listbox.items.size() - 1)
						.saved_attribute("parameter_initial_enable", 1)
						.saved_attribute("parameter_initial", defaultIndex)
						.saved_attribute("parameter_longname", "filterbankDecay[1]") // todo
						.saved_attribute("parameter_shortname", listbox.name)
						.saved_attribute("parameter_enum", listbox.items)
						.saved_attribute("parameter_type", 2)
						.saved_attribute("parameter_linknames", 1)
						.end();
				patching_y += 60;
				
				if (elem.listbox.oscAddress.empty() == false)
				{
					const std::string osc_id = allocObjectId();
					patchEditor
						.beginBox(osc_id.c_str(), 1, 1)
							.patching_rect(patching_x, patching_y, 40, 22) // automatic height will be 22 for 4d.paramOsc
							.text(String::FormatC("4d.paramOsc %s", listbox.oscAddress.c_str()).c_str())
							.end();
					patching_y += 60;
					
					connect(osc_id, 0, listbox_id, 0);
					connect(listbox_id, 0, osc_id, 0);
				}
			}
		}
		
		patching_x += 300;
		patching_y = 10;
	}
#endif
	
	rapidjson::StringBuffer buffer;
	REFLECTIONIO_JSON_WRITER json_writer(buffer);
	
	object_tojson_recursive(typeDB, typeDB.findType(patch), &patch, json_writer);
	
	FILE * f = fopen("test.maxpat", "wt");
	
	if (f != nullptr)
	{
		fprintf(f, "%s", buffer.GetString());
		
		fclose(f);
		f = nullptr;
	}
	
	return 0;
}

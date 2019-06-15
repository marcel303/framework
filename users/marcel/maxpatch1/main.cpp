// max objects includes
#include "reflection.h"
#include "reflection-jsonio.h"
#include <string>
#include <vector>

// UI app includes
#include "framework.h"
#include "osc/OscOutboundPacketStream.h"
#include "oscSender.h"

// Max/MSP generator app includes
#include "framework.h"
#include "StringEx.h"
#include <stdio.h>
#include <unistd.h> // fixme

// todo : move UI generator to a separate source file
// todo : refine UI generator
// todo : move Max/MSP patch generator to its own source file
// todo : determine a way to include Max/MSP patch snippets, so complicated stuff can be generated externally
// todo : add reflection type UI structure
// todo : create UI app, which reads reflected UI structure from file and allows knob control, OSC output and creating presets

namespace max
{
	struct AppVersion
	{
		int major = 7;
		int minor = 2;
		int revision = 0;
		
		static void reflect(TypeDB & typeDB)
		{
			typeDB.addStructured<max::AppVersion>("max::AppVersion")
				.add("major", &AppVersion::major)
				.add("minor", &AppVersion::minor)
				.add("revision", &AppVersion::revision);
		}
	};
	
	struct SavedAttribute
	{
		std::string name;
		std::string value;
	};
	
	struct SavedAttributeAttributes
	{
		std::vector<SavedAttribute> valueof;
		
		static void reflect(TypeDB & typeDB)
		{
		// todo : custom json serialization flag should be set here
			typeDB.addStructured<max::SavedAttributeAttributes>("max::SavedAttributeAttributes");
		}
	};
	
	static bool savedAttributeAttributesToJson(const TypeDB & typeDB, const void * object, const Member * member, REFLECTIONIO_JSON_WRITER & writer)
	{
		SavedAttributeAttributes * attrs = (SavedAttributeAttributes*)object;
		
		writer.StartObject();
		{
			writer.Key("valueof");
			writer.StartObject();
			{
				for (auto & attr : attrs->valueof)
				{
					writer.Key(attr.name.c_str());
					writer.String(attr.value.c_str());
				}
			}
			writer.EndObject();
		}
		writer.EndObject();
		
		return true;
	}
	
	struct Box
	{
		std::string comment;
		std::string id; // unique id
		std::string maxclass = "newobj";
		int numinlets = 0;
		int numoutlets = 0;
		std::vector<std::string> outlettype;
		std::vector<float> patching_rect = { 0, 0, 10, 10 };
		bool presentation = false;
		std::vector<float> presentation_rect = { 0, 0, 10, 10 };
		std::string text; // this contains the arguments passed to the node
		
		// this member requires custom serialization, since Max stores it rather oddly.. not as structured data
		SavedAttributeAttributes saved_attribute_attributes;
		bool parameter_enable = false;
		std::string varname;
		
		// todo : a box may contain a sub-patcher (Patcher patcher)
		
		static void reflect(TypeDB & typeDB)
		{
			typeDB.addStructured<max::Box>("max::Box")
				.add("comment", &Box::comment)
				.add("id", &Box::id)
				.add("maxclass", &Box::maxclass)
				.add("numinlets", &Box::numinlets)
				.add("numoutlets", &Box::numoutlets)
				.add("outlettype", &Box::outlettype)
				.add("patching_rect", &Box::patching_rect)
				.add("presentation", &Box::presentation)
				.add("presentation_rect", &Box::presentation_rect)
				.add("text", &Box::text)
				.add("saved_attribute_attributes", &Box::saved_attribute_attributes)
					.addFlag(customJsonSerializationFlag(savedAttributeAttributesToJson))
				.add("parameter_enable", &Box::parameter_enable)
				.add("varname", &Box::varname);
		}
	};
	
	struct Line
	{
		std::vector<std::string> destination = { "", "0" }; // name and index
		bool disabled = false;
		bool hidden = false;
		std::vector<std::string> source = { "", "0" }; // name and index
		
		static void reflect(TypeDB & typeDB)
		{
			typeDB.addStructured<max::Line>("max::Line")
				.add("destination", &Line::destination)
				.add("disabled", &Line::disabled)
				.add("hidden", &Line::hidden)
				.add("source", &Line::source);
		}
	};
	
	// --- Patcher ---
	
	struct PatchBox
	{
		Box box;
		
		static void reflect(TypeDB & typeDB)
		{
			typeDB.addStructured<max::PatchBox>("max::PatchBox")
				.add("box", &PatchBox::box);
		}
	};
	
	struct PatchLine
	{
		Line patchline;
		
		static void reflect(TypeDB & typeDB)
		{
			typeDB.addStructured<max::PatchLine>("max::PatchLine")
				.add("patchline", &PatchLine::patchline);
		}
	};
	
	struct Patcher
	{
		int fileversion = 1;
		AppVersion appversion;
		std::vector<float> rect = { 0, 0, 800, 600 };
		bool openinpresentation = true;
		float default_fontsize = 12.f;
		int default_fontface = 0;
		std::string default_fontname = "Andale Mono";
		std::string description;
		std::vector<PatchBox> boxes;
		std::vector<PatchLine> lines;
		
		static void reflect(TypeDB & typeDB)
		{
			typeDB.addStructured<max::Patcher>("max::Patcher")
				.add("fileversion", &Patcher::fileversion)
				.add("appversion", &Patcher::appversion)
				.add("rect", &Patcher::rect)
				.add("openinpresentation", &Patcher::openinpresentation)
				.add("default_fontsize", &Patcher::default_fontsize)
				.add("default_fontface", &Patcher::default_fontface)
				.add("default_fontname", &Patcher::default_fontname)
				.add("description", &Patcher::description)
				.add("boxes", &Patcher::boxes)
				.add("lines", &Patcher::lines);
		}
	};
	
	struct Patch
	{
		Patcher patcher;
		
		static void reflect(TypeDB & typeDB)
		{
			typeDB.addStructured<max::Patch>("max::Patch")
				.add("patcher", &Patch::patcher);
		}
	};
}

struct PatchBuilder_MainInterface;
struct PatchBuilder_GroupInterface;
struct PatchBuilder_KnobInterface;

struct PatchBuilder
{
	enum ElemType
	{
		kElemType_None,
		kElemType_Knob
	};
	
	struct Elem
	{
		ElemType type = kElemType_None;
		int x = 0;
		int y = 0;
		int sx = 0;
		int sy = 0;
	};
	
	struct Knob : Elem
	{
		Knob()
		{
			type = kElemType_Knob;
			
			sx = 40;
			sy = 40;
		}
		
		std::string name;
		float defaultValue = 0.f;
		bool hasDefaultValue = false;
		float min = 0.f;
		float max = 1.f;
		float exponential = 1.f;
		std::string oscAddress;
	};
	
	struct Group
	{
		std::string name;
		std::vector<Elem*> elems;
	};
	
	struct Layout
	{
		int sx = 0;
		int sy = 0;
		
		int marginX = 0;
		int marginY = 0;
		
		int paddingX = 0;
		int paddingY = 0;
	};
	
	struct LayoutInterface
	{
		PatchBuilder * patch = nullptr;
		
		LayoutInterface & size(const int sx, const int sy)
		{
			patch->layout.sx = sx;
			patch->layout.sy = sy;
			return *this;
		}
		
		LayoutInterface & margin(const int x, const int y)
		{
			patch->layout.marginX = x;
			patch->layout.marginY = y;
			return *this;
		}
		
		LayoutInterface & padding(const int x, const int y)
		{
			patch->layout.paddingX = x;
			patch->layout.paddingY = y;
			return *this;
		}
		
		void layoutEnd()
		{
			patch->layoutEnd();
		}
	};
	
	std::vector<Group*> groups;
	
	Layout layout;
	
	void initializeDefaultValues()
	{
		for (auto * group : groups)
		{
			for (auto * elem : group->elems)
			{
				if (elem->type == kElemType_Knob)
				{
					auto * knob = static_cast<Knob*>(elem);
					
					if (knob->hasDefaultValue == false)
					{
						knob->defaultValue = knob->min;
						knob->hasDefaultValue = true;
					}
				}
			}
		}
	}
	
	LayoutInterface layoutBegin()
	{
		LayoutInterface layout;
		layout.patch = this;
		
		return layout;
	}
	
	void layoutEnd()
	{
		int x = layout.marginX;
		int y = layout.marginY;
		
		for (auto * group : groups)
		{
			int sx = 0;
			
			for (auto * elem : group->elems)
			{
				const bool fits = y + elem->sy + layout.paddingY <= layout.sy - layout.marginY;
				
				if (fits == false)
				{
					x += sx;
					y = layout.marginY;
					
					sx = 0;
				}
				
				elem->x = x;
				elem->y = y;
				
				y += elem->sy + layout.paddingY;
				
				if (elem->sx > sx)
					sx = elem->sx + layout.paddingX;
			}
			
			// end the current group
			
			x += sx;
			y = layout.marginY;
			
			sx = 0;
		}
	}
};

struct PatchBuilder_MainInterface
{
	PatchBuilder * patch = nullptr;
	
	PatchBuilder_MainInterface(PatchBuilder * in_patch)
		: patch(in_patch)
	{
	}
	
	PatchBuilder_GroupInterface pushGroup(const char * name);
};

struct PatchBuilder_GroupInterface
{
	PatchBuilder_MainInterface patch;
	PatchBuilder::Group * group = nullptr;
	
	PatchBuilder_GroupInterface(const PatchBuilder_MainInterface & in_patch, PatchBuilder::Group * in_group)
		: patch(in_patch)
		, group(in_group)
	{
	}
	
	PatchBuilder_KnobInterface pushKnob(const char * name);
	
	PatchBuilder_MainInterface popGroup();
	
	void name(const char * name)
	{
	}
};

struct PatchBuilder_KnobInterface
{
	PatchBuilder_GroupInterface group;
	PatchBuilder::Knob * knob = nullptr;
	
	PatchBuilder_KnobInterface(const PatchBuilder_GroupInterface & in_group, PatchBuilder::Knob * in_knob)
		: group(in_group)
		, knob(in_knob)
	{
	}
	
	PatchBuilder_KnobInterface & name(const char * name)
	{
		knob->name = name;
		
		return *this;
	}
	
	PatchBuilder_KnobInterface & defaultValue(const float defaultValue)
	{
		knob->defaultValue = defaultValue;
		knob->hasDefaultValue = true;
		
		return *this;
	}
	
	PatchBuilder_KnobInterface & limits(const float min, const float max)
	{
		knob->min = min;
		knob->max = max;
		
		return *this;
	}
	
	PatchBuilder_KnobInterface & exponential(const float exponential)
	{
		knob->exponential = exponential;
		
		return *this;
	}
	
	PatchBuilder_KnobInterface & osc(const char * address)
	{
		knob->oscAddress = address;
		
		return *this;
	}
	
	PatchBuilder_GroupInterface popKnob();
};

//

PatchBuilder_GroupInterface PatchBuilder_MainInterface::pushGroup(const char * name)
{
	auto * group = new PatchBuilder::Group();
	group->name = name;
	
	patch->groups.push_back(group);

	return PatchBuilder_GroupInterface(patch, group);
}

//

PatchBuilder_KnobInterface PatchBuilder_GroupInterface::pushKnob(const char * name)
{
	auto * knob = new PatchBuilder::Knob();
	knob->name = name;
	
	group->elems.push_back(knob);
	
	return PatchBuilder_KnobInterface(*this, knob);
}

PatchBuilder_MainInterface PatchBuilder_GroupInterface::popGroup()
{
	return patch;
}

//


PatchBuilder_GroupInterface PatchBuilder_KnobInterface::popKnob()
{
	return group;
}

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
	
	//
	
#if 1
	{
		PatchBuilder _patch;
		
		PatchBuilder_MainInterface patch(&_patch);
	
		patch
			.pushGroup("master")
				.pushKnob("intensity")
					.defaultValue(5.f)
					.limits(0.f, 10.f)
					.exponential(2.f)
					.osc("/master/intensity")
					.popKnob()
				.pushKnob("VU")
					.limits(0.f, 1.f)
					.exponential(2.f)
					.osc("/master/vu")
					.popKnob()
				.pushKnob("A/B")
					.limits(0.f, 1.f)
					.exponential(2.f)
					.osc("/master/ab")
					.popKnob()
				.popGroup()
		
			.pushGroup("source")
				.pushKnob("position")
					.limits(0.f, 1.f)
					.exponential(2.f)
					.popKnob()
				.pushKnob("speed")
					.limits(0.f, 1.f)
					.exponential(2.f)
					.popKnob()
				.pushKnob("scale")
					.limits(0.f, 1.f)
					.exponential(2.f)
					.popKnob()
				.pushKnob("position")
					.limits(0.f, 1.f)
					.exponential(2.f)
					.popKnob()
				.pushKnob("speed")
					.limits(0.f, 1.f)
					.exponential(2.f)
					.popKnob()
				.pushKnob("scale")
					.limits(0.f, 1.f)
					.exponential(2.f)
					.popKnob()
				.popGroup();
		
		_patch.initializeDefaultValues();
		
		_patch.layoutBegin()
			.size(800, 200)
			.margin(10, 10)
			.padding(4, 4)
			.layoutEnd();
		
		struct LiveUi
		{
			struct Elem
			{
				PatchBuilder::Elem * elem = nullptr;
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
			
			void addElem(PatchBuilder::Elem * elem)
			{
				elems.resize(elems.size() + 1);
				
				auto & e = elems.back();
				e.elem = elem;
				
				if (elem->type == PatchBuilder::kElemType_Knob)
				{
					auto * knob = static_cast<PatchBuilder::Knob*>(elem);
					Assert(knob->hasDefaultValue);
					
					if (knob->hasDefaultValue)
					{
						const float t = (knob->defaultValue - knob->min) / (knob->max - knob->min);
						
						e.value = powf(t, 1.f / knob->exponential);
						e.defaultValue = e.value;
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
					
					if (elem->type == PatchBuilder::kElemType_Knob)
					{
						auto * knob = static_cast<PatchBuilder::Knob*>(elem);
						
						if (activeElem == nullptr && isInside && mouse.wentDown(BUTTON_LEFT))
						{
							activeElem = &e;
							SDL_CaptureMouse(SDL_TRUE);
							
							if (e.doubleClickTimer > 0.f)
							{
								if (knob->hasDefaultValue)
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
							
							e.value = saturate<float>(e.value + mouse.dy * speed);
							
							if (e.value != oldValue)
							{
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
						
						if (e.elem->type == PatchBuilder::kElemType_Knob)
						{
							auto * knob = static_cast<PatchBuilder::Knob*>(e.elem);
							
							if (!knob->oscAddress.empty())
							{
								const float t = powf(activeElem->value, knob->exponential);
								const float value = knob->min * (1.f - t) + knob->max * t;
								
								if (s.Size() + knob->oscAddress.size() + 100 > 1200)
								{
									sendBundle();
									beginBundle();
								}
								
								s << osc::BeginMessage(knob->oscAddress.c_str());
								{
									s << value;
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
					
					if (elem->type == PatchBuilder::kElemType_Knob)
					{
						auto * knob = static_cast<PatchBuilder::Knob*>(elem);
						
						hqBegin(HQ_FILLED_ROUNDED_RECTS);
						{
							if (&e == activeElem)
								setLumi(190);
							else if (&e == hoverElem)
								setLumi(210);
							else
								setLumi(200);
							hqFillRoundedRect(knob->x, knob->y, knob->x + knob->sx, knob->y + knob->sy, 4);
						}
						hqEnd();
						
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
						setFont("calibri.ttf");
						drawText(knob->x + knob->sx / 2.f, knob->y + knob->sy - 2, 10, 0, -1, "%s", knob->name.c_str());
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
						
						if (elem->type == PatchBuilder::kElemType_Knob)
						{
							auto * knob = static_cast<PatchBuilder::Knob*>(elem);
							
							hqBegin(HQ_FILLED_ROUNDED_RECTS);
							{
								setColor(200, 200, 100);
								hqFillRoundedRect(0, 0, 100, 30, 4);
							}
							hqEnd();
							
							const float t = powf(activeElem->value, knob->exponential);
							const float value = knob->min * (1.f - t) + knob->max * t;
							
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
		
		for (auto * group : _patch.groups)
		{
			for (auto * elem : group->elems)
			{
				liveUi.addElem(elem);
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
				liveUi.draw();
				
				liveUi.drawTooltip();
			}
			framework.endDraw();
		}
	}
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
	
	std::string previousId;
	
	for (int i = 0; i < 100; ++i)
	{
		max::PatchBox box;
		box.box.comment = "My First Box";
		box.box.id = allocObjectId();
		box.box.maxclass = "newobj";
		box.box.numinlets = 1;
		box.box.numoutlets = 1;
		box.box.patching_rect = { 10, i * 30.f, 40, 20 };
		box.box.presentation = true;
		box.box.presentation_rect = { 10, i * 30.f, 40, 20 };
		box.box.text = "gate";
		patch.patcher.boxes.push_back(box);
		
		if (!previousId.empty())
		{
			connect(previousId, 0, box.box.id, 0);
		}
		
		previousId = box.box.id;
	}
	
	for (int i = 0; i < 10; ++i)
	{
		std::string osc_id;
		std::string knob_id;
		
		{
			max::PatchBox box;
			box.box.id = allocObjectId();
			box.box.numinlets = 1;
			box.box.numoutlets = 1;
			box.box.patching_rect = { 100, 30 + i * 100.f, 40, 20 };
			box.box.text = "4d.paramOsc /filterbankPlayerMidiChannel";
			patch.patcher.boxes.push_back(box);
			
			osc_id = box.box.id;
		}
		
		{
			max::PatchBox box;
			box.box.id = allocObjectId();
			box.box.maxclass = "live.dial";
			box.box.numinlets = 1;
			box.box.numoutlets = 2;
			box.box.patching_rect = { 100, 60 + i * 100.f, 40, 20 };
			box.box.presentation = true;
			box.box.presentation_rect = { 100 + i * 40.f, 60, 40, 20 };
			box.box.parameter_enable = true;
			box.box.varname = "filterbankDecay"; // script variable name
			
			box.box.saved_attribute_attributes.valueof =
			{
				{ "parameter_mmin", "1.0" },
				{ "parameter_mmax", "10.0" },
				{ "parameter_exponent", "2.0" },
				{ "parameter_longname", "filterbankDecay[1]" },
				{ "parameter_shortname",  "decay" },
				{ "parameter_type",  "0" },
				{ "parameter_unitstyle", "1" },
				{ "parameter_linknames", "1" }
			};
			
			patch.patcher.boxes.push_back(box);
			
			knob_id = box.box.id;
		}
		
		connect(osc_id, 0, knob_id, 0);
		connect(knob_id, 0, osc_id, 0);
	}
	
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

#include "framework.h"
#include "imgui-framework.h"

struct ResonVertex
{
	bool isFixed = false;
	
	Vec3 p;
	Vec3 v;
	Vec3 f;
	
	float pressure = 0.f;
};

struct ResonEdge
{
	int vertex1;
	int vertex2;
	
	float desiredDistance;
	
	float strength;
	
	ResonEdge(const int in_vertex1, const int in_vertex2, const float in_strength = 1.f)
		: vertex1(in_vertex1)
		, vertex2(in_vertex2)
		, desiredDistance(1.f)
		, strength(in_strength)
	{
	}
};

struct ResonShape
{
	std::vector<ResonVertex> vertices;
	std::vector<ResonEdge> edges;
	
	void step(const float dt)
	{
		const float C = 1.f * 1000.f;
		
		for (auto & vertex : vertices)
		{
			vertex.f.SetZero();
			
			vertex.pressure = 0.f;
		}
		
		for (auto & edge : edges)
		{
			ResonVertex & v1 = vertices[edge.vertex1];
			ResonVertex & v2 = vertices[edge.vertex2];
			
			const Vec3 delta = v2.p - v1.p;
			const Vec3 direction = delta.CalcNormalized();
			const float distance = delta.CalcSize();
			
			if (distance < 1e-3f)
				continue;
			
			const float stretchedDistance = distance - edge.desiredDistance;
			
			v1.f += direction * stretchedDistance * C * edge.strength;
			v2.f -= direction * stretchedDistance * C * edge.strength;
			
			v1.pressure -= stretchedDistance * edge.strength;
			v1.pressure -= stretchedDistance * edge.strength;
		}
		
		const float falloff = powf(.8f, dt);
		
		for (auto & vertex : vertices)
		{
			if (vertex.isFixed)
				continue;
			
			vertex.v += vertex.f * dt;
			vertex.p += vertex.v * dt;
			
			vertex.v *= falloff;
		}
	}
	
	void reset()
	{
		for (auto & vertex : vertices)
			vertex.v.SetZero();
	}
};

static void buildPlate(ResonShape & shape, const int gridSx, const int gridSy, const float physicalSize)
{
	shape.vertices.resize(gridSx * gridSy);
	
	// setup vertices
	
	const int gridSize = gridSx > gridSy ? gridSx : gridSy;
	
	for (int x = 0; x < gridSx; ++x)
	{
		for (int y = 0; y < gridSy; ++y)
		{
			const int index = x + y * gridSx;
			
			auto & vertex = shape.vertices[index];
			
			vertex.p[0] = (x + .5f) * physicalSize / gridSize;
			vertex.p[1] = (y + .5f) * physicalSize / gridSize;
			
			//vertex.isFixed = (rand() % 20) == 0;
			//vertex.isFixed = y == gridSy - 1;
		}
	}
	
	// setup edges
	
	for (int x = 0; x < gridSx - 1; ++x)
	{
		for (int y = 0; y < gridSy - 1; ++y)
		{
			const int index00 = (x + 0) + (y + 0) * gridSx;
			const int index10 = (x + 1) + (y + 0) * gridSx;
			const int index11 = (x + 1) + (y + 1) * gridSx;
			const int index01 = (x + 0) + (y + 1) * gridSx;
			
		#if 1
			shape.edges.emplace_back(ResonEdge(index00, index10));
			shape.edges.emplace_back(ResonEdge(index10, index11));
			shape.edges.emplace_back(ResonEdge(index11, index01));
			shape.edges.emplace_back(ResonEdge(index01, index00));
		#endif
			
		#if 1
			shape.edges.emplace_back(ResonEdge(index00, index11, 1.f / sqrtf(2.f)));
			shape.edges.emplace_back(ResonEdge(index10, index01, 1.f / sqrtf(2.f)));
		#endif
		}
	}
	
	for (auto & edge : shape.edges)
	{
		const auto & vertex1 = shape.vertices[edge.vertex1];
		const auto & vertex2 = shape.vertices[edge.vertex2];
		
		const Vec3 delta = vertex2.p - vertex1.p;
		const float distance = delta.CalcSize();
		
		edge.desiredDistance = distance;
	}
}

struct ResponseProbe
{
	double frequency = 1.0;
	double phase = 0.0;
	
	double responseX = 0.0;
	double responseY = 0.0;
	
	void init(const double in_frequency)
	{
		frequency = in_frequency;
		phase = 0.0;
		
		responseX = 0.0;
		responseY = 0.0;
	}
	
	void next(const double value, const double dt)
	{
		phase += dt * frequency;
		
		if (phase >= 1.0)
			phase -= 1.0;
		
		responseX += value * cos(phase * 2.0 * M_PI);
		responseY += value * sin(phase * 2.0 * M_PI);
	}
	
	double calcResponse() const
	{
		return hypot(responseX, responseY);
	}
};

int main(int argc, char * argv[])
{
#if defined(CHIBI_RESOURCE_PATH)
	changeDirectory(CHIBI_RESOURCE_PATH);
#endif

	if (!framework.init(640, 480))
		return -1;

	ResonShape shape;
	
	int gridSx = 13;
	int gridSy = 23;
	
	const float physicalSize = .5f;
	buildPlate(shape, gridSx, gridSy, physicalSize);
	
	double time_ms = 0.0;
	
	const int kNumProbeLayers = 23;
	const int kNumProbes = 64;
	ResponseProbe probes[kNumProbeLayers][kNumProbes];
	for (int l = 0; l < kNumProbeLayers; ++l)
		for (int i = 0; i < kNumProbes; ++i)
			probes[l][i].init(40.0 * pow(2.0, i / 8.0));
	
	bool applyResonantExcitation = false;
	double resonantExcitationPhase = 0.0;
	
	FrameworkImGuiContext guiContext;
	guiContext.init();
	
	bool interact_pullDown = false;
	bool interact_contract = true;
	bool interact_random00 = false;
	bool interact_random = false;
	
	while (!framework.quitRequested)
	{
		framework.process();
		
		if (keyboard.wentDown(SDLK_ESCAPE))
			framework.quitRequested = true;
		
		bool inputIsCaptured = false;
		
		guiContext.processBegin(framework.timeStep, 640, 480, inputIsCaptured);
		{
			ImGui::Begin("Interaction");
			{
				const bool reset = ImGui::Button("Reset");
				
				ImGui::Checkbox("Pull down", &interact_pullDown);
				ImGui::Checkbox("Contract", &interact_contract);
				ImGui::Checkbox("Randomize (0, 0)", &interact_random00);
				ImGui::Checkbox("Randomize", &interact_random);
				
				const int oldSx = gridSx;
				const int oldSy = gridSy;
				ImGui::InputInt("Grid Width", &gridSx);
				ImGui::InputInt("Grid Height", &gridSy);
				
				gridSx = clamp(gridSx, 1, 50);
				gridSy = clamp(gridSy, 1, 50);
				
				if (reset || gridSx != oldSx || gridSy != oldSy)
				{
					shape = ResonShape();
					
					buildPlate(shape, gridSx, gridSy, physicalSize);
					
					for (auto & probeArray : probes)
						for (auto & probe : probeArray)
							probe.init(probe.frequency);
					
					time_ms = 0.0;
				}
			}
			ImGui::End();
		}
		guiContext.processEnd();
		
		if (inputIsCaptured == false && keyboard.wentDown(SDLK_f))
		{
			inputIsCaptured = true;
			applyResonantExcitation = !applyResonantExcitation;
		}
		
		const int numIterations = 100;
		
		double dt_ms = framework.timeStep / (double)numIterations; // note : multiply with 1000.0 to get real-time result
		
		//dt_ms *= 1000.0;
		//dt_ms *= 10.0;
		//dt_ms /= 10.0;
		
		bool applyInteraction = false;
		
		if (inputIsCaptured == false && mouse.isDown(BUTTON_LEFT))
		{
			inputIsCaptured = true;
			applyInteraction = true;
		}
		
		for (int i = 0; i < numIterations; ++i)
		{
			if (applyInteraction)
			{
				const Vec3 mid = Vec3(physicalSize/2.f, physicalSize/2.f, 0.f);
				
				for (auto & vertex : shape.vertices)
				{
					Vec3 force;
					
					if (interact_random)
					{
						force[0] += random(-1.f, +1.f) * 100.f;
						force[1] += random(-1.f, +1.f) * 100.f;
					}
				
					if (interact_random00)
					{
						if (vertex.p[0] < physicalSize*3/4.f && vertex.p[1] < physicalSize*1/4.f)
							force[0] += random(0.f, 1.f) * 100.f;
					}
				
					if (interact_contract)
					{
						vertex.p = (vertex.p - mid) * pow(0.8, dt_ms) + mid;
					}
				
					if (interact_pullDown)
					{
						if (vertex.p[0] < physicalSize / 10.f)
							force[1] += 1.f * 100.f;
					}
				
					vertex.p += force * dt_ms / 1000.f;
				}
				
			#if 0
			if (mouse.wentDown(BUTTON_LEFT))
				shape.vertices[0].p[0] = physicalSize / gridSx;
			#endif
			}
			
			if (applyResonantExcitation)
			{
				resonantExcitationPhase += dt_ms * 100.0 / 1000.0;
				if (resonantExcitationPhase >= 1.0)
					resonantExcitationPhase -= 1.0;
				
				shape.vertices[0].p[0] = sin(resonantExcitationPhase * 2.0 * M_PI) / 2000.0 * dt_ms;
			}
			
			shape.step(dt_ms); // in milliseconds
			time_ms += dt_ms;
			
			//
			
			for (int l = 0; l < kNumProbeLayers; ++l)
			{
				const int x = l * gridSx / kNumProbeLayers;
				
				const int index = x + gridSy*2/3 * gridSx;
				
				//const float value = shape.vertices[gridSize/3 + gridSize/3 * gridSize].v.CalcSize();
				//const float value = shape.vertices[index].v[0];
				const float value = shape.vertices[index].v[1];
				//const float value = shape.vertices[index].pressure;
			
				for (int i = 0; i < kNumProbes; ++i)
					probes[l][i].next(value, dt_ms / 1000.0);
			}
		}
		
		framework.beginDraw(0, 0, 0, 0);
		{
			setFont("calibri.ttf");
			pushFontMode(FONT_SDF);
			
			gxPushMatrix();
			{
				const int gridSize = gridSx > gridSy ? gridSx : gridSy;
				
				gxTranslatef(320, 240, 0);
				gxScalef(400 / physicalSize, 400 / physicalSize, 1);
				gxTranslatef(- gridSx / float(gridSize) * physicalSize/2.f, - gridSy / float(gridSize) * physicalSize/2.f, 0);
				
			#if 1
				// draw edges connecting vertices
				
				hqBegin(HQ_LINES, true);
				{
					setColor(colorWhite);
					
					for (auto & edge : shape.edges)
					{
						const auto & vertex1 = shape.vertices[edge.vertex1];
						const auto & vertex2 = shape.vertices[edge.vertex2];
						
						hqLine(vertex1.p[0], vertex1.p[1], 1.f, vertex2.p[0], vertex2.p[1], 1.f);
					}
				}
				hqEnd();
			#endif
			
			#if 1
				// draw colored vertices
				
				hqBegin(HQ_FILLED_CIRCLES, true);
				{
					setColor(colorWhite);
					
					for (const auto & vertex : shape.vertices)
					{
						setColorf(.5f + vertex.v[0] * 10.f, .5f + vertex.v[1] * 10.f, 0.f);
						//setLumif(.5f + vertex.pressure * 10.f);
						hqFillCircle(vertex.p[0], vertex.p[1], 10.f);
					}
				}
				hqEnd();
			#endif
			}
			gxPopMatrix();
			
			setColor(colorWhite);
			drawText(8, 460, 12, +1, +1, "time %gms", time_ms);
			
			// draw response graph
			
			const int graphSx = 600;
			const int graphSy = 200;
			
			hqBegin(HQ_FILLED_ROUNDED_RECTS);
			{
				// draw the background

				setColor(0, 0, 0, 127);
				hqFillRoundedRect(0, 0, graphSx, graphSy, 8.f);
			}
			hqEnd();
			
			// calculate the responses
			
			double responses[kNumProbeLayers][kNumProbes];
			for (int l = 0; l < kNumProbeLayers; ++l)
				for (int i = 0; i < kNumProbes; ++i)
					responses[l][i] = probes[l][i].calcResponse();
			double maxResponse = 0.0;
			for (int l = 0; l < kNumProbeLayers; ++l)
				for (int i = 0; i < kNumProbes; ++i)
					maxResponse = fmax(maxResponse, responses[l][i]);
			
			for (int l = 0; l < kNumProbeLayers; ++l)
			{
				if (keyboard.isDown(SDLK_LSHIFT))
				{
					const int selectedLayer = mouse.y * kNumProbeLayers / 480;
					if (l != selectedLayer)
						continue;
				}
				
				gxPushMatrix();
				gxTranslatef(l * 3, l * 3, 0);
				hqBegin(HQ_LINES);
				{
					// draw graph lines
					
					setColor(Color::fromHSL(.5f, l / float(kNumProbeLayers), .5f));
					
					for (int i = 0; i < kNumProbes - 1; ++i)
					{
						const double response1 = responses[l][i + 0];
						const double response2 = responses[l][i + 1];
						const float strokeSize = 2.f;
						hqLine(
							(i + 0) * double(graphSx) / kNumProbes, double(graphSy) - response1 * double(graphSy) / maxResponse, strokeSize,
							(i + 1) * double(graphSx) / kNumProbes, double(graphSy) - response2 * double(graphSy) / maxResponse, strokeSize);
					}
				}
				hqEnd();
				gxPopMatrix();
			}
			
			setColor(colorWhite);
			for (int i = 0; i < kNumProbes; i += kNumProbes / 10)
				drawText((i + 0) * double(graphSx) / kNumProbes, 4, 12, +1, +1, "%dHz", (int)probes[0][i].frequency);
			
			guiContext.draw();
			
			popFontMode();
		}
		framework.endDraw();
	}
	
	guiContext.shut();
	
	framework.shutdown();
	
	return 0;
}

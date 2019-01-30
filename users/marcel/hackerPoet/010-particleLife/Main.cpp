#include "framework.h"
#include "Universe.h"
#include <iostream>

/*
Framework port of Hacker Poet's 'Particle Life',
see: https://github.com/HackerPoet/Particle-Life
*/

static const int window_w = 1200;
static const int window_h = 900;
static const int steps_per_frame_normal = 10;

int main(int argc, char *argv[]) {
	std::cout << "=========================================================" << std::endl;
	std::cout << std::endl;
	std::cout << "               Welcome to Particle Life" << std::endl;
	std::cout << std::endl;
	std::cout << "  This is a particle-based game of life simulation based" << std::endl;
	std::cout << "on random attraction and repulsion between all particle" << std::endl;
	std::cout << "classes.  For more details about how this works and other" << std::endl;
	std::cout << "fun projects, check out my YouTube channel 'CodeParade'." << std::endl;
	std::cout << std::endl;
	std::cout << "=========================================================" << std::endl;
	std::cout << std::endl;
	std::cout << "Controls:" << std::endl;
	std::cout << "         'B' - Randomize (Balanced)" << std::endl;
	std::cout << "         'C' - Randomize (Chaos)" << std::endl;
	std::cout << "         'D' - Randomize (Diversity)" << std::endl;
	std::cout << "         'F' - Randomize (Frictionless)" << std::endl;
	std::cout << "         'G' - Randomize (Gliders)" << std::endl;
	std::cout << "         'H' - Randomize (Homogeneity)" << std::endl;
	std::cout << "         'L' - Randomize (Large Clusters)" << std::endl;
	std::cout << "         'M' - Randomize (Medium Clusters)" << std::endl;
	std::cout << "         'Q' - Randomize (Quiescence)" << std::endl;
	std::cout << "         'S' - Randomize (Small Clusters)" << std::endl;
	std::cout << "         'W' - Toggle Wrap-Around" << std::endl;
	std::cout << "       Enter - Keep rules, but re-seed particles" << std::endl;
	std::cout << "       Space - Hold for slow-motion" << std::endl;
	std::cout << "         Tab - Print current parameters to console" << std::endl;
	std::cout << "  Left Click - Click a particle to follow it" << std::endl;
	std::cout << " Right Click - Click anywhere to unfollow particle" << std::endl;
	std::cout << "Scroll Wheel - Zoom in/out" << std::endl;
	std::cout << std::endl;
	system("pause");

	//Create the universe of particles
	Universe universe(9, 400, window_w, window_h);
	universe.ReSeed(-0.02f, 0.06f, 0.0f, 20.0f, 20.0f, 70.0f, 0.05f, false);

	//Camera settings
	float cam_x = float(window_w/2);
	float cam_y = float(window_h/2);
	float cam_zoom = 1.0f;
	float cam_x_dest = cam_x;
	float cam_y_dest = cam_y;
	float cam_zoom_dest = cam_zoom;
	float last_scroll_time = 0.f;
	int track_index = -1;
	int steps_per_frame = steps_per_frame_normal;

	if (!framework.init(window_w, window_h))
		return -1;
	
	//Main Loop
	while (!framework.quitRequested)
	{
		framework.process();
		
		if (keyboard.wentDown(SDLK_ESCAPE)) {
			framework.quitRequested = true;
		}
		if (keyboard.wentDown(SDLK_b)) { //Balanced
			universe.SetPopulation(9, 400);
			universe.ReSeed(-0.02f, 0.06f, 0.0f, 20.0f, 20.0f, 70.0f, 0.05f, false);
		}
		if (keyboard.wentDown(SDLK_c)) { //Chaos
			universe.SetPopulation(6, 400);
			universe.ReSeed(0.02f, 0.04f, 0.0f, 30.0f, 30.0f, 100.0f, 0.01f, false);
		}
		if (keyboard.wentDown(SDLK_d)) { //Diversity
			universe.SetPopulation(12, 400);
			universe.ReSeed(-0.01f, 0.04f, 0.0f, 20.0f, 10.0f, 60.0f, 0.05f, true);
		}
		if (keyboard.wentDown(SDLK_f)) { //Frictionless
			universe.SetPopulation(6, 300);
			universe.ReSeed(0.01f, 0.005f, 10.0f, 10.0f, 10.0f, 60.0f, 0.0f, true);
		}
		if (keyboard.wentDown(SDLK_g)) { //Gliders
			universe.SetPopulation(6, 400);
			universe.ReSeed(0.0f, 0.06f, 0.0f, 20.0f, 10.0f, 50.0f, 0.1f, true);
		}
		if (keyboard.wentDown(SDLK_h)) { //Homogeneity
			universe.SetPopulation(4, 400);
			universe.ReSeed(0.0f, 0.04f, 10.0f, 10.0f, 10.0f, 80.0f, 0.05f, true);
		}
		if (keyboard.wentDown(SDLK_l)) { //Large Clusters
			universe.SetPopulation(6, 400);
			universe.ReSeed(0.025f, 0.02f, 0.0f, 30.0f, 30.0f, 100.0f, 0.2f, false);
		}
		if (keyboard.wentDown(SDLK_m)) { //Medium Clusters
			universe.SetPopulation(6, 400);
			universe.ReSeed(0.02f, 0.05f, 0.0f, 20.0f, 20.0f, 50.0f, 0.05f, false);
		}
		if (keyboard.wentDown(SDLK_q)) { //Quiescence
			universe.SetPopulation(6, 300);
			universe.ReSeed(-0.02f, 0.1f, 10.0f, 20.0f, 20.0f, 60.0f, 0.2f, false);
		}
		if (keyboard.wentDown(SDLK_s)) { //Small Clusters
			universe.SetPopulation(6, 600);
			universe.ReSeed(-0.005f, 0.01f, 10.0f, 10.0f, 20.0f, 50.0f, 0.01f, false);
		}
		if (keyboard.wentDown(SDLK_w)) {
			universe.ToggleWrap();
		}
		if (keyboard.wentDown(SDLK_RETURN)) {
			universe.SetRandomParticles();
		}
		if (keyboard.wentDown(SDLK_TAB)) {
			universe.PrintParams();
		}
		if (keyboard.wentDown(SDLK_SPACE)) {
			steps_per_frame = 1;
		}
		if (keyboard.wentUp(SDLK_SPACE)) {
			steps_per_frame = steps_per_frame_normal;
		}
		
		cam_zoom_dest *= std::pow(1.1f, mouse.scrollY);
		cam_zoom_dest = std::max(std::min(cam_zoom_dest, 10.0f), 1.0f);
		const float cur_time = framework.time;
		if (cur_time - last_scroll_time > .3f) {
			//Only update position if scroll just started
			const Vec2 mouse_pos = Vec2(mouse.x, mouse.y);
			universe.ToCenter(mouse_pos[0], mouse_pos[1], cam_x_dest, cam_y_dest);
		}
		last_scroll_time = cur_time;
		
		if (mouse.wentDown(BUTTON_LEFT)) {
			track_index = universe.GetIndex(mouse.x, mouse.y);
		}
		if (mouse.wentDown(BUTTON_RIGHT)) {
			track_index = -1;
		}
	
		//Apply zoom
		if (track_index >= 0) {
			cam_x_dest = universe.GetParticleX(track_index);
			cam_y_dest = universe.GetParticleY(track_index);
		}
		cam_x = cam_x*0.9f + cam_x_dest*0.1f;
		cam_y = cam_y*0.9f + cam_y_dest*0.1f;
		cam_zoom = cam_zoom*0.8f + cam_zoom_dest*0.2f;
		universe.Zoom(cam_x, cam_y, cam_zoom);

		//Apply physics and draw
		framework.beginDraw(0, 0, 0, 0);
		{
			for (int i = 0; i < steps_per_frame; ++i) {
				const float opacity = float(i + 1) / float(steps_per_frame);
				universe.Step();
				universe.Draw(opacity);
			}
		}
		framework.endDraw();
	}

	framework.shutdown();
	
	return 0;
}

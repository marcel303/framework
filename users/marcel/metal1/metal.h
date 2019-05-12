#pragma once

struct SDL_Window;

void metal_init();
void metal_attach(SDL_Window * window);
void metal_make_active(SDL_Window * window);
void metal_draw_begin(const float r, const float g, const float b, const float a);
void metal_draw_end();

void metal_drawtest();

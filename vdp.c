// Commander X16 Emulator
// Copyright (c) 2019 Michael Steil
// All rights reserved. License: 2-clause BSD

#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <stdlib.h>
#include "vdp.h"
#include "memory.h"
//XXX
#include "glue.h"


static SDL_Window *window;
static SDL_Renderer *renderer;
static SDL_Texture *sdlTexture;

static uint8_t framebuffer[SCREEN_WIDTH * SCREEN_HEIGHT * 4];

static uint8_t vdp_v_reg[47];
static uint8_t vdp_s_reg[10];

bool
vdp_step(float mhz)
{
	return false; // true - new_frame
}

bool 
vdp_update(){
//	printf("vdp_update()\n");
			
	SDL_UpdateTexture(sdlTexture, NULL, framebuffer, SCREEN_WIDTH * 4);
	SDL_RenderClear(renderer);
	SDL_RenderCopy(renderer, sdlTexture, NULL, NULL);
	SDL_RenderPresent(renderer);
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_QUIT) {
			return false;
			printf("vdp_update()\n %x", event.type);
		}
	}	
	return true;
}

void
vdp_reset()
{
}

bool
vdp_init(int window_scale, char *quality)
{
	srand(time(NULL));
	vdp_reset();

	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, quality);
	SDL_CreateWindowAndRenderer(SCREEN_WIDTH * window_scale, SCREEN_HEIGHT * window_scale, 0, &window, &renderer);
#ifndef __MORPHOS__
	SDL_SetWindowResizable(window, true);
#endif
	SDL_RenderSetLogicalSize(renderer, SCREEN_WIDTH, SCREEN_HEIGHT);

	sdlTexture = SDL_CreateTexture(renderer,
									SDL_PIXELFORMAT_RGB888,
									SDL_TEXTUREACCESS_STREAMING,
									SCREEN_WIDTH, SCREEN_HEIGHT);

	SDL_SetWindowTitle(window, "Steckschwein");
/*
	if (record_gif != RECORD_GIF_DISABLED) {
		if (!strcmp(gif_path+strlen(gif_path)-5, ",wait")) {
			// wait for POKE
			record_gif = RECORD_GIF_PAUSED;
			// move the string terminator to remove the ",wait"
			gif_path[strlen(gif_path)-5] = 0;
		} else {
			// start now
			record_gif = RECORD_GIF_ACTIVE;
		}
		if (!GifBegin(&gif_writer, gif_path, SCREEN_WIDTH, SCREEN_HEIGHT, 1, 8, false)) {
			record_gif = RECORD_GIF_DISABLED;
		}
	}
*/
	if (debugger_enabled) {
	//	DEBUGInitUI(renderer);
	}

	return true;
}

uint8_t
vdp_read(uint8_t reg)
{
	if (reg > 9)
	{
		fprintf(stderr, "attempted to read non-existent status register R#%d\n", reg);
		return -1;
	}
	return vdp_s_reg[reg];
}

void
vdp_write(uint8_t reg, uint8_t value)
{
	if (reg > 46)
	{
		fprintf(stderr, "attempted to write to non-existent R#%d\n", reg);
		return;
	}
	vdp_v_reg[reg] = value;
	// TODO
}

void vdp_end(){
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
}
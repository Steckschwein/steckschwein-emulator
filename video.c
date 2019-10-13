// Commander X16 Emulator
// Copyright (c) 2019 Michael Steil
// All rights reserved. License: 2-clause BSD

#include "video.h"
#include "memory.h"
#include "glue.h"
#include "debugger.h"
#include "gif.h"

#ifdef __EMSCRIPTEN__
#include "emscripten.h"
#endif

#define ESC_IS_BREAK /* if enabled, Esc sends Break/Pause key instead of Esc */

// visible area we're darwing
#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480

#ifdef __APPLE__
#define LSHORTCUT_KEY SDL_SCANCODE_LGUI
#define RSHORTCUT_KEY SDL_SCANCODE_RGUI
#else
#define LSHORTCUT_KEY SDL_SCANCODE_LCTRL
#define RSHORTCUT_KEY SDL_SCANCODE_RCTRL
#endif

static SDL_Window *window;
static SDL_Renderer *renderer;
static SDL_Texture *sdlTexture;
static bool is_fullscreen = false;

static uint8_t framebuffer[SCREEN_WIDTH * SCREEN_HEIGHT * 4];

static GifWriter gif_writer;

void
video_reset()
{
}

bool
video_init(int window_scale, char *quality)
{
	video_reset();

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

	SDL_SetWindowTitle(window, "Commander X16");

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

	if (debugger_enabled) {
		DEBUGInitUI(renderer);
	}

	return true;
}

#define EXTENDED_FLAG 0x100

int
ps2_scancode_from_SDLKey(SDL_Scancode k)
{
	switch (k) {
		case SDL_SCANCODE_GRAVE:
			return 0x0e;
		case SDL_SCANCODE_BACKSPACE:
			return 0x66;
		case SDL_SCANCODE_TAB:
			return 0xd;
		case SDL_SCANCODE_CLEAR:
			return 0;
		case SDL_SCANCODE_RETURN:
			return 0x5a;
		case SDL_SCANCODE_PAUSE:
			return 0;
		case SDL_SCANCODE_ESCAPE:
#ifdef ESC_IS_BREAK
			return 0xff;
#else
			return 0x76;
#endif
		case SDL_SCANCODE_SPACE:
			return 0x29;
		case SDL_SCANCODE_APOSTROPHE:
			return 0x52;
		case SDL_SCANCODE_COMMA:
			return 0x41;
		case SDL_SCANCODE_MINUS:
			return 0x4e;
		case SDL_SCANCODE_PERIOD:
			return 0x49;
		case SDL_SCANCODE_SLASH:
			return 0x4a;
		case SDL_SCANCODE_0:
			return 0x45;
		case SDL_SCANCODE_1:
			return 0x16;
		case SDL_SCANCODE_2:
			return 0x1e;
		case SDL_SCANCODE_3:
			return 0x26;
		case SDL_SCANCODE_4:
			return 0x25;
		case SDL_SCANCODE_5:
			return 0x2e;
		case SDL_SCANCODE_6:
			return 0x36;
		case SDL_SCANCODE_7:
			return 0x3d;
		case SDL_SCANCODE_8:
			return 0x3e;
		case SDL_SCANCODE_9:
			return 0x46;
		case SDL_SCANCODE_SEMICOLON:
			return 0x4c;
		case SDL_SCANCODE_EQUALS:
			return 0x55;
		case SDL_SCANCODE_LEFTBRACKET:
			return 0x54;
		case SDL_SCANCODE_BACKSLASH:
			return 0x5d;
		case SDL_SCANCODE_RIGHTBRACKET:
			return 0x5b;
		case SDL_SCANCODE_A:
			return 0x1c;
		case SDL_SCANCODE_B:
			return 0x32;
		case SDL_SCANCODE_C:
			return 0x21;
		case SDL_SCANCODE_D:
			return 0x23;
		case SDL_SCANCODE_E:
			return 0x24;
		case SDL_SCANCODE_F:
			return 0x2b;
		case SDL_SCANCODE_G:
			return 0x34;
		case SDL_SCANCODE_H:
			return 0x33;
		case SDL_SCANCODE_I:
			return 0x43;
		case SDL_SCANCODE_J:
			return 0x3B;
		case SDL_SCANCODE_K:
			return 0x42;
		case SDL_SCANCODE_L:
			return 0x4B;
		case SDL_SCANCODE_M:
			return 0x3A;
		case SDL_SCANCODE_N:
			return 0x31;
		case SDL_SCANCODE_O:
			return 0x44;
		case SDL_SCANCODE_P:
			return 0x4D;
		case SDL_SCANCODE_Q:
			return 0x15;
		case SDL_SCANCODE_R:
			return 0x2D;
		case SDL_SCANCODE_S:
			return 0x1B;
		case SDL_SCANCODE_T:
			return 0x2C;
		case SDL_SCANCODE_U:
			return 0x3C;
		case SDL_SCANCODE_V:
			return 0x2A;
		case SDL_SCANCODE_W:
			return 0x1D;
		case SDL_SCANCODE_X:
			return 0x22;
		case SDL_SCANCODE_Y:
			return 0x35;
		case SDL_SCANCODE_Z:
			return 0x1A;
		case SDL_SCANCODE_DELETE:
			return 0;
		case SDL_SCANCODE_UP:
			return 0x75 | EXTENDED_FLAG;
		case SDL_SCANCODE_DOWN:
			return 0x72 | EXTENDED_FLAG;
		case SDL_SCANCODE_RIGHT:
			return 0x74 | EXTENDED_FLAG;
		case SDL_SCANCODE_LEFT:
			return 0x6b | EXTENDED_FLAG;
		case SDL_SCANCODE_INSERT:
			return 0;
		case SDL_SCANCODE_HOME:
			return 0x6c | EXTENDED_FLAG;
		case SDL_SCANCODE_END:
			return 0;
		case SDL_SCANCODE_PAGEUP:
			return 0;
		case SDL_SCANCODE_PAGEDOWN:
			return 0;
		case SDL_SCANCODE_F1:
			return 0x05;
		case SDL_SCANCODE_F2:
			return 0x06;
		case SDL_SCANCODE_F3:
			return 0x04;
		case SDL_SCANCODE_F4:
			return 0x0c;
		case SDL_SCANCODE_F5:
			return 0x03;
		case SDL_SCANCODE_F6:
			return 0x0b;
		case SDL_SCANCODE_F7:
			return 0x83;
		case SDL_SCANCODE_F8:
			return 0x0a;
		case SDL_SCANCODE_F9:
			return 0x01;
		case SDL_SCANCODE_F10:
			return 0x09;
		case SDL_SCANCODE_F11:
			return 0x78;
		case SDL_SCANCODE_F12:
			return 0x07;
		case SDL_SCANCODE_RSHIFT:
			return 0x59;
		case SDL_SCANCODE_LSHIFT:
			return 0x12;
		case SDL_SCANCODE_LCTRL:
			return 0x14;
		case SDL_SCANCODE_RCTRL:
			return 0x14 | EXTENDED_FLAG;
		case SDL_SCANCODE_LALT:
			return 0x11;
		case SDL_SCANCODE_RALT:
			return 0x11 | EXTENDED_FLAG;
//		case SDL_SCANCODE_LGUI: // Windows/Command
//			return 0x5b | EXTENDED_FLAG;
		case SDL_SCANCODE_NONUSBACKSLASH:
			return 0x61;
		case SDL_SCANCODE_KP_ENTER:
			return 0x5a | EXTENDED_FLAG;
		case SDL_SCANCODE_KP_0:
			return 0x70;
		case SDL_SCANCODE_KP_1:
			return 0x69;
		case SDL_SCANCODE_KP_2:
			return 0x72;
		case SDL_SCANCODE_KP_3:
			return 0x7a;
		case SDL_SCANCODE_KP_4:
			return 0x6b;
		case SDL_SCANCODE_KP_5:
			return 0x73;
		case SDL_SCANCODE_KP_6:
			return 0x74;
		case SDL_SCANCODE_KP_7:
			return 0x6c;
		case SDL_SCANCODE_KP_8:
			return 0x75;
		case SDL_SCANCODE_KP_9:
			return 0x7d;
		case SDL_SCANCODE_KP_PERIOD:
			return 0x71;
		case SDL_SCANCODE_KP_PLUS:
			return 0x79;
		case SDL_SCANCODE_KP_MINUS:
			return 0x7b;
		case SDL_SCANCODE_KP_MULTIPLY:
			return 0x7c;
		case SDL_SCANCODE_KP_DIVIDE:
			return 0x4a | EXTENDED_FLAG;
		default:
			return 0;
	}
}

bool
video_step(float mhz)
{
	return false; // new_frame
}

bool
video_get_irq_out()
{
	return false; // isr > 0;
}

//
// saves the video memory and register content into a file
//

void
video_save(FILE *f)
{
}

void
kbd_buffer_add(uint8_t c)
{
	// TODO
}

bool
video_update()
{
	SDL_UpdateTexture(sdlTexture, NULL, framebuffer, SCREEN_WIDTH * 4);

	if (record_gif > RECORD_GIF_PAUSED) {
		if(!GifWriteFrame(&gif_writer, framebuffer, SCREEN_WIDTH, SCREEN_HEIGHT, 2, 8, false)) {
			// if that failed, stop recording
			GifEnd(&gif_writer);
			record_gif = RECORD_GIF_DISABLED;
			printf("Unexpected end of recording.\n");
		}
		if (record_gif == RECORD_GIF_SINGLE) { // if single-shot stop recording
			record_gif = RECORD_GIF_PAUSED;  // need to close in video_end()
		}
	}

	SDL_RenderClear(renderer);
	SDL_RenderCopy(renderer, sdlTexture, NULL, NULL);

	if (debugger_enabled && showDebugOnRender != 0) {
		DEBUGRenderDisplay(SCREEN_WIDTH, SCREEN_HEIGHT);
		SDL_RenderPresent(renderer);
		return true;
	}

	SDL_RenderPresent(renderer);

	static bool cmd_down = false;
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_QUIT) {
			return false;
		}
		if (event.type == SDL_KEYDOWN) {
			bool consumed = false;
			if (cmd_down) {
				if (event.key.keysym.sym == SDLK_s) {
					machine_dump();
					consumed = true;
				} else if (event.key.keysym.sym == SDLK_r) {
					machine_reset();
					consumed = true;
				} else if (event.key.keysym.sym == SDLK_v) {
					machine_paste(SDL_GetClipboardText());
					consumed = true;
				} else if (event.key.keysym.sym == SDLK_f || event.key.keysym.sym == SDLK_RETURN) {
					is_fullscreen = !is_fullscreen;
					SDL_SetWindowFullscreen(window, is_fullscreen ? SDL_WINDOW_FULLSCREEN : 0);
					consumed = true;
				}
			}
			if (!consumed) {
				if (log_keyboard) {
					printf("DOWN 0x%02X\n", event.key.keysym.scancode);
					fflush(stdout);
				}
				if (event.key.keysym.scancode == LSHORTCUT_KEY || event.key.keysym.scancode == RSHORTCUT_KEY) {
					cmd_down = true;
				}

				int scancode = ps2_scancode_from_SDLKey(event.key.keysym.scancode);
				if (scancode == 0xff) {
					// "Pause/Break" sequence
					kbd_buffer_add(0xe1);
					kbd_buffer_add(0x14);
					kbd_buffer_add(0x77);
					kbd_buffer_add(0xe1);
					kbd_buffer_add(0xf0);
					kbd_buffer_add(0x14);
					kbd_buffer_add(0xf0);
					kbd_buffer_add(0x77);
				} else {
					if (scancode & EXTENDED_FLAG) {
						kbd_buffer_add(0xe0);
					}
					kbd_buffer_add(scancode & 0xff);
				}
			}
			return true;
		}
		if (event.type == SDL_KEYUP) {
			if (log_keyboard) {
				printf("UP   0x%02X\n", event.key.keysym.scancode);
				fflush(stdout);
			}
			if (event.key.keysym.scancode == LSHORTCUT_KEY || event.key.keysym.scancode == RSHORTCUT_KEY) {
				cmd_down = false;
			}

			int scancode = ps2_scancode_from_SDLKey(event.key.keysym.scancode);
			if (scancode & EXTENDED_FLAG) {
				kbd_buffer_add(0xe0);
			}
			kbd_buffer_add(0xf0); // BREAK
			kbd_buffer_add(scancode & 0xff);
			return true;
		}
	}
	return true;
}

void
video_end()
{
	if (debugger_enabled) {
		DEBUGFreeUI();
	}

	if (record_gif != RECORD_GIF_DISABLED) {
		GifEnd(&gif_writer);
		record_gif = RECORD_GIF_DISABLED;
	}

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
}

//
// Video: 6502 I/O Interface
//
// if debugOn, read without any side effects (registers & memory unchanged)

uint8_t
video_read(uint8_t reg, bool debugOn)
{
	switch (reg) {
		default:
			return 0;
	}
}

void
video_write(uint8_t reg, uint8_t value)
{
//	printf("ioregisters[%d] = $%02X\n", reg, value);
	switch (reg) {
		default:
			break;
	}
}

void
video_update_title(const char* window_title)
{
	SDL_SetWindowTitle(window, window_title);
}


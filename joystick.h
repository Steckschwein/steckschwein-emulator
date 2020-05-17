/**********************************************/
// File     :     joystick.h
// Author   :     John Bliss
// Date     :     September 27th 2019
/**********************************************/

#ifndef _JOYSTICK_H_
#define _JOYSTICK_H_

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <SDL.h>
#include "glue.h"
#include "via.h"

#if SDL_MAJOR_VERSION == 1
/**
 *  from sdl 2 SDL_gamecontroller.h
 */
typedef enum
{
    SDL_CONTROLLER_BUTTON_INVALID = -1,
    SDL_CONTROLLER_BUTTON_A,
    SDL_CONTROLLER_BUTTON_B,
    SDL_CONTROLLER_BUTTON_X,
    SDL_CONTROLLER_BUTTON_Y,
    SDL_CONTROLLER_BUTTON_BACK,
    SDL_CONTROLLER_BUTTON_GUIDE,
    SDL_CONTROLLER_BUTTON_START,
    SDL_CONTROLLER_BUTTON_LEFTSTICK,
    SDL_CONTROLLER_BUTTON_RIGHTSTICK,
    SDL_CONTROLLER_BUTTON_LEFTSHOULDER,
    SDL_CONTROLLER_BUTTON_RIGHTSHOULDER,
    SDL_CONTROLLER_BUTTON_DPAD_UP,
    SDL_CONTROLLER_BUTTON_DPAD_DOWN,
    SDL_CONTROLLER_BUTTON_DPAD_LEFT,
    SDL_CONTROLLER_BUTTON_DPAD_RIGHT,
    SDL_CONTROLLER_BUTTON_MAX
} SDL_GameControllerButton;

#endif


//see snes.inc
#define JOY_CLK_MASK 1<<0
#define JOY_LATCH_MASK 1<<1
#define JOY_DATA1_MASK 1<<2
#define JOY_DATA2_MASK 1<<3

enum joy_status { NES=0, NONE=1, SNES=0xF };

extern enum joy_status joy1_mode;
extern enum joy_status joy2_mode;
extern bool joystick_latch, joystick_clock;
extern bool joystick1_data, joystick2_data;

bool joystick_init(); //initialize SDL controllers

void joystick_step(); //do next step for handling joysticks

bool handle_latch(bool latch, bool clock);  //used internally to check when to write to VIA

//Used to get the 16-bit data needed to send
uint16_t get_joystick_state(SDL_Joystick *control, enum joy_status mode);

#endif

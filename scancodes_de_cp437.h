
// Control Characters
#define C_SOH 0x01 // Ctrl-A, Start of heading
#define C_STX 0x02 // Ctrl-B, Start of text
#define C_ETX 0x03 // Ctrl-C, End of text
#define C_EOT 0x04 // Ctrl-D, End of transmission
#define C_ENQ 0x05 // Ctrl-E, Enquiry
#define C_BEL 0x07 // Ctrl-G, Bell
#define C_BS  0x08 // Ctrl-H, Backspace


// control keys
#define CRSR_UP 	 0x1E // RS (Record Separator)
#define CRSR_DOWN 	 0x1F // US (Unit separator)
#define CRSR_RIGHT 	 0x10 // DLE (Data Link Escape)
#define CRSR_LEFT 	 0x11 // DC1 (Device Control 1
#define KEY_HOME 	 0x12 // DC2
#define KEY_END 	 0x13 // DC3
#define KEY_DEL 	 0x14 // DC4
#define KEY_ESC		 0x1b // Escape Key

// function keys
#define FUNC_F1     0xF1
#define FUNC_F2     0xF2
#define FUNC_F3     0xF3
#define FUNC_F4     0xF4
#define FUNC_F5     0xF5
#define FUNC_F6     0xF6
#define FUNC_F7     0xF7
#define FUNC_F8     0xF8
#define FUNC_F9     0xF9
#define FUNC_F10     0xFA
#define FUNC_F11     0xFB
#define FUNC_F12     0xFC

#include <SDL_keycode.h>

const unsigned char scancodes[][4] = {

	[SDLK_BACKSPACE]	= {8, 8, 0, 8},
	[SDLK_TAB]			= { 9, 9 ,0 ,9},
	[SDL_SCANCODE_CLEAR]		= {12, 12, 0, 12},
	[SDLK_RETURN]		= {13, 13,0,13},
	[SDLK_ESCAPE]		= {27,27,0,27},
	[SDL_SCANCODE_PAUSE]		= {19,19,0,19},

	[SDLK_0] = {'0', '=', 0, '}'} ,
	[SDLK_1] = {'1', '!', 0, 0} ,
	[SDLK_2] = {'2', '"', 0, 0} ,
	[SDLK_3] = {'3', 0x15, 0, 0} , //�
	[SDLK_4] = {'4', '$', 0, 0} ,
	[SDLK_5] = {'5', '%', 0, 0} ,
	[SDLK_6] = {'6', '&', 0, 0} ,
	[SDLK_7] = {'7', '/', 0, '{'} ,
	[SDLK_8] = {'8', '(', 0, '['} ,
	[SDLK_9] = {'9', ')', 0, ']'} ,

	[SDLK_SPACE] = {' ', ' ', 0, 0} ,

	[SDLK_a] = {'a', 'A', C_SOH, 0},
	[SDLK_b] = {'b', 'B', C_STX, 0} ,
	[SDLK_c] = {'c', 'C', C_ETX, 0} ,
	[SDLK_d] = {'d', 'D', C_EOT, 4} ,
	[SDLK_e] = {'e', 'E', C_ENQ , 5} ,
	[SDLK_f] = {'f', 'F', 0, 0} ,
	[SDLK_g] = {'g', 'G', C_BEL, 0} ,
	[SDLK_h] = {'h', 'H', C_BS, 0} ,
	[SDLK_i] = {'i', 'I', 0, 0} ,
	[SDLK_j] = {'j', 'J', 0, 0} ,
	[SDLK_k] = {'k', 'K', 0, 0} ,
	[SDLK_l] = {'l', 'L', 0, 0} ,
	[SDLK_m] = {'m', 'M', 0, 0xE6} , //æ
	[SDLK_n] = {'n', 'N', 0, 0} ,
	[SDLK_o] = {'o', 'O', 0, 0xf8},
	[SDLK_p] = {'p', 'P', 0, 0} ,
	[SDLK_q] = {'q', 'Q', 0, '@'},
	[SDLK_r] = {'r', 'R', 0, 0} ,
	[SDLK_s] = {'s', 'S', 0, 0} ,
  [SDLK_t] = {'t', 'T', 0, 0} ,
	[SDLK_u] = {'u', 'U', 0, 0} ,
	[SDLK_v] = {'v', 'V', 0, 0} ,
	[SDLK_w] = {'w', 'W', 0, 0} ,
	[SDLK_x] = {'x', 'X', 0, 0} ,
	[SDLK_y] = {'y', 'Y', 0, 0} ,
	[SDLK_z] = {'z', 'Z', 0, 0} ,
	[SDLK_COMMA] = {',', ';', 0, 0} ,
	[SDLK_PERIOD] = {'.', ':', 0, 0} ,
	[SDLK_MINUS] = {'-', '_', 0, 0} ,
	[SDLK_PLUS] = {'+', '*', 0, '~'},
	[SDLK_LESS] = {'<', '>', 0, '|'},
	[SDLK_HASH] = {'#', '\'', 0, '~'},
	[SDLK_QUOTE] = {'\'', '`', 0, 0},
	[228] = {0xe4, 0xc4, 0, 0},// ä
	[246] = {0xf6, 0xd6, 0, 0},// ö
	[252] = {0xfc, 0xdc, 0, 0},// ü
	[223] = {0xdf, '?', 0, '\\'}// ß
};

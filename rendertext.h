
#ifndef _RENDERTEXT_H
#define _RENDERTEXT_H

#include <SDL.h>
#include <ctype.h>

#define CHAR_SCALE 		(1)										// character pixel size.

extern int xPos;
extern int yPos;

void DEBUGInitChars(SDL_Surface *renderer);
void DEBUGWrite(SDL_Surface *renderer, int x, int y, int ch, SDL_Color colour);
void DEBUGString(SDL_Surface *renderer, int x, int y, char *s, SDL_Color colour);
void DEBUGDestroy();

char *ltrim(char *s);

#endif

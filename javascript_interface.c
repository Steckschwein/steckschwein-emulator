// Commander X16 Emulator
// Copyright (c) 2019 Michael Steil
// All rights reserved. License: 2-clause BSD

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "glue.h"

char javascript_text_data[65536];

int semCount=0;

int SDL_SetTimer(int interval, void* callback){
	printf("SDL_SetTimer\n");
}

void SDL_KillThread(void *thread){
	printf("SDL_KillThread %p\n", thread);
}

void * SDL_CreateSemaphore(int count)
{
	printf("SDL_CreateSemaphore %x\n", count);
	semCount = count;
	return &semCount;
}

int SDL_SemPost(void* s)
{
	printf("SDL_SemPost %p\n", s);
	return ++semCount;
}

int SDL_SemWait(void* s)
{
	printf("SDL_SemWait %p\n", s);
	return (semCount > 0 ? semCount-- : -1);
}

void
j2c_reset()
{
	printf("SDL_KillThread\n");
  machine_reset();
}

void
j2c_paste(char * buffer)
{
	memset(javascript_text_data, 0, 65536);
	strcpy(javascript_text_data, buffer);
	machine_paste(javascript_text_data);
}

void
j2c_start_audio()
{

}

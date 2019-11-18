#include "Board.h"

typedef enum { EMU_RUNNING, EMU_PAUSED, EMU_STOPPED, EMU_SUSPENDED, EMU_STEP, EMU_STEP_BACK } EmuState;

void actionEmuTogglePause();
void actionEmuStepBack();

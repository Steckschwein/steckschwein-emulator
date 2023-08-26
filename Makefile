#
# Comment out if verbose comilation is wanted
#
SILENT = @

#
# windows 10 build
#   CROSS_COMPILE_WINDOWS=1 WIN_SDL=<sdl home> CC=<gcc home>/mingw32-gcc make clean all
#
ifndef (MINGW32)
	# the mingw32 path on macOS installed through homebrew
	MINGW32=/usr/local/Cellar/mingw-w64/6.0.0_2/toolchain-i686/i686-w64-mingw32
endif


# Flags
#
# production flags (performance)
CFLAGS   = -w -O3 -DLSB_FIRST -DNO_FILE_HISTORY -DNO_EMBEDDED_SAMPLES -Wall -Werror -fomit-frame-pointer

# development flags (debugger support)
# CFLAGS   = -g -w -DLSB_FIRST -DNO_FILE_HISTORY -DNO_EMBEDDED_SAMPLES -Wall -Werror
# CFLAGS   +=-DDEBUG_ENABLED
# Videorenderer.c segfault inline asm, we disable it entirely
CFLAGS   +=-DNO_ASM

#CFLAGS   += -DSINGLE_THREADED
#CFLAGS   += -DNO_TIMERS
#CFLAGS   += -DNO_HIRES_TIMERS
#CFLAGS   += -DTRACE
#CFLAGS   += -DEMU_AVR_KEYBOARD_IRQ
#CFLAGS   += -DTRACE_RTC

# ym3812 opl sound
CFLAGS +=-DBUILD_YM3812

# switch compile ssw 2.0 architecture
CFLAGS +=-DSSW2_0

CFLAGS +=-mcmodel=large
#
# SDL specific flags
#
SDL_PREFIX=
ifdef WIN_SDL
	SDL_PREFIX=--prefix=$(WIN_SDL)
	#LDFLAGS+=-L$(MINGW32)/lib
	# this enables printf() to show, but also forces a console window
	LDFLAGS+=-mconsole -Wl,--subsystem,console
else
	# the Windows SDL path on macOS installed through ./configure --prefix=... && make && make install
	WIN_SDL=~/tmp/sdl-win32
endif

ifeq ($(CROSS_COMPILE_WINDOWS),1)
	SDLCONFIG=$(WIN_SDL)/bin/sdl2-config
else
	SDLCONFIG=sdl2-config
endif

CFLAGS+=$(shell $(SDLCONFIG) $(SDL_PREFIX) --cflags)
LDFLAGS+=$(shell $(SDLCONFIG) $(SDL_PREFIX) --libs) -lm

ifdef CROSS_COMPILE_WINDOWS
	CFLAGS+=-Wno-error=deprecated-declarations
#	CFLAGS+=-Wno-error=incompatible-pointer-types
	CFLAGS+=-Wno-error=maybe-uninitialized
#	CFLAGS+=-DENABLE_VRAM_DECAY \
	CFLAGS+=-D_REENTRANT
else
	CFLAGS+=-std=c99
endif

ifeq ($(MAC_STATIC),1)
	LDFLAGS=/usr/local/lib/libSDL.a -lm -liconv -Wl,-framework,CoreAudio -Wl,-framework,AudioToolbox -Wl,-framework,ForceFeedback -lobjc -Wl,-framework,CoreVideo -Wl,-framework,Cocoa -Wl,-framework,Carbon -Wl,-framework,IOKit -Wl,-weak_framework,QuartzCore -Wl,-weak_framework,Metal
endif

ifeq ($(CROSS_COMPILE_WINDOWS),1)
	LDFLAGS+=-L$(MINGW32)/lib
	# this enables printf() to show, but also forces a console window
	LDFLAGS+=-static-libgcc -static-libstdc++ -mconsole -Wl,--subsystem,console
endif

LIBS     = #-lz
TARGET   = steckschwein-emu

SRCS        = $(SOURCE_FILES)
OBJS        = $(patsubst %.rc,%.res,$(patsubst %.cxx,%.o,$(patsubst %.cpp,%.o,$(patsubst %.cc,%.o,$(patsubst %.c,%.o,$(filter %.c %.cc %.cpp %.cxx %.rc,$(SRCS)))))))
OUTPUT_OBJS = $(addprefix $(OUTPUT_DIR)/, $(OBJS))

ifdef EMSCRIPTEN
	SOURCE_FILES += javascript_interface.c

	LDFLAGS+=--shell-file webassembly/steckschwein-emu-template.html
	LDFLAGS+=--preload-file rom.bin
#	LDFLAGS+=-s ASYNCIFY
	LDFLAGS+=-s TOTAL_MEMORY=64MB
	LDFLAGS+=-s ALLOW_MEMORY_GROWTH=1
	LDFLAGS+=-s ASSERTIONS=1
#	LDFLAGS+=-s EXIT_RUNTIME=1
 	LDFLAGS+=-s DISABLE_DEPRECATED_FIND_EVENT_TARGET_BEHAVIOR=1
	# To the Javascript runtime exported functions
	LDFLAGS+=-s EXPORTED_FUNCTIONS='["_SDL_SetTimer", "_SDL_WaitEvent", "_SDL_CreateSemaphore", "_SDL_DestroySemaphore", "_SDL_SemPost", "_SDL_SemWait", "_j2c_reset", "_j2c_paste", "_j2c_start_audio", _main]'
	LDFLAGS+=-s EXPORTED_RUNTIME_METHODS='["ccall", "cwrap"]'
#	LDFLAGS+=-s LLD_REPORT_UNDEFINED
#	LDFLAGS+=-s ERROR_ON_UNDEFINED_SYMBOLS=0
#	LDFLAGS+=-s WASM=0

	TARGET=steckschwein-emu.html
endif

ifneq ("$(wildcard ./rom_labels.h)","")
	HEADERS+=rom_labels.h
endif

#
# Tools
#
RM    = $(SILENT)-rm -f
RMDIR = $(SILENT)-rm -rf
MKDIR = $(SILENT)-mkdir
ECHO  = @echo

ROOT_DIR   = .
OUTPUT_DIR = objs

EXTERN_BLUEMSX_DIR=$(ROOT_DIR)/extern/blueMSX

#
# Include paths
#
CFLAGS += -I$(ROOT_DIR)
CFLAGS += -I$(ROOT_DIR)/extern/include
CFLAGS += -I$(ROOT_DIR)/extern/src
# blue msx include stuff
CFLAGS += -I$(EXTERN_BLUEMSX_DIR)/Src/Arch
CFLAGS += -I$(EXTERN_BLUEMSX_DIR)/Src/Board
CFLAGS += -I$(EXTERN_BLUEMSX_DIR)/Src/Common
CFLAGS += -I$(EXTERN_BLUEMSX_DIR)/Src/Cpu
CFLAGS += -I$(EXTERN_BLUEMSX_DIR)/Src/Debugger
CFLAGS += -I$(EXTERN_BLUEMSX_DIR)/Src/Emulator
CFLAGS += -I$(EXTERN_BLUEMSX_DIR)/Src/Memory
CFLAGS += -I$(EXTERN_BLUEMSX_DIR)/Src/Sdl
CFLAGS += -I$(EXTERN_BLUEMSX_DIR)/Src/SoundChips
CFLAGS += -I$(EXTERN_BLUEMSX_DIR)/Src/VideoChips
CFLAGS += -I$(EXTERN_BLUEMSX_DIR)/Src/VideoRender

#vpath % $(ROOT_DIR)
vpath % $(ROOT_DIR)/cpu
vpath % $(ROOT_DIR)/extern/src
vpath % $(EXTERN_BLUEMSX_DIR)/Src/Arch
vpath % $(EXTERN_BLUEMSX_DIR)/Src/Board
vpath % $(EXTERN_BLUEMSX_DIR)/Src/Common
vpath % $(EXTERN_BLUEMSX_DIR)/Src/Cpu
vpath % $(EXTERN_BLUEMSX_DIR)/Src/Debugger
vpath % $(EXTERN_BLUEMSX_DIR)/Src/Emulator
vpath % $(EXTERN_BLUEMSX_DIR)/Src/Memory
vpath % $(EXTERN_BLUEMSX_DIR)/Src/Sdl
vpath % $(EXTERN_BLUEMSX_DIR)/Src/SoundChips
vpath % $(EXTERN_BLUEMSX_DIR)/Src/VideoChips
vpath % $(EXTERN_BLUEMSX_DIR)/Src/VideoRender

#
# Source files
#
SOURCE_FILES += EmulatorDebugger.c
SOURCE_FILES += rendertext.c
SOURCE_FILES += disasm.c
SOURCE_FILES += main.c
SOURCE_FILES += memory.c
SOURCE_FILES += ym3812.c
SOURCE_FILES += sdcard.c
SOURCE_FILES += spi.c
SOURCE_FILES += uart.c
SOURCE_FILES += via.c
SOURCE_FILES += joystick.c

# rtc sound
SOURCE_FILES += ds1306.c

# cpu 65x02
SOURCE_FILES += fake6502.c

# blueMSX Src
SOURCE_FILES += SdlEvent.c
SOURCE_FILES += SdlSound.c
SOURCE_FILES += SdlTimer.c
SOURCE_FILES += SdlThread.c
SOURCE_FILES += SdlVideoIn.c
SOURCE_FILES += FrameBuffer.c
SOURCE_FILES += VDP.c
SOURCE_FILES += V9938.c
SOURCE_FILES += VideoManager.c
SOURCE_FILES += hq2x.c
SOURCE_FILES += hq3x.c
SOURCE_FILES += Scalebit.c
SOURCE_FILES += VideoRender.c

SOURCE_FILES += Fmopl.c
SOURCE_FILES += Ymdeltat.c
SOURCE_FILES += AudioMixer.c
SOURCE_FILES += Actions.c
SOURCE_FILES += Properties.c
SOURCE_FILES += Board.c
SOURCE_FILES += IoPort.c
SOURCE_FILES += Steckschwein.c
SOURCE_FILES += MOS6502.c
SOURCE_FILES += DebugDeviceManager.c
SOURCE_FILES += Debugger.c

all: $(OUTPUT_DIR) $(TARGET)

$(TARGET): $(OUTPUT_OBJS)
	$(ECHO) Linking $@... $(CC) $(LDFLAGS) -o $@ $(OUTPUT_OBJS) $(LIBS)
	$(SILENT)$(CC) -o $@ $(OUTPUT_OBJS) $(LDFLAGS) $(LIBS)

clean: clean_$(TARGET)

clean_$(TARGET):
	$(ECHO) Cleaning files ...
	$(RMDIR) -rf $(OUTPUT_DIR)
	$(RM) -f $(TARGET)
	$(RM) -f *.o cpu/*.o extern/src/*.o steckschwein-emu.js steckschwein-emu.wasm steckschwein-emu.data steckschwein-emu.worker.js steckschwein-emu.html steckschwein-emu.html.mem
	$(RM) -f extern/blueMSX/objs/*.o

cpu/tables.h cpu/mnemonics.h: cpu/buildtables.py cpu/6502.opcodes cpu/65c02.opcodes
	cd cpu && python buildtables.py

$(OUTPUT_DIR):
	$(ECHO) Creating directory $@...
	$(MKDIR) $(OUTPUT_DIR)

$(OUTPUT_DIR)/%.o: %.c
	$(ECHO) Compiling $<...
	$(SILENT)$(CC) $(CFLAGS) $(INCLUDE) -o $@ -c $<

#$(OUTPUT_DIR)/%.o: %.cc
#	$(ECHO) Compiling $<...
#	$(CXX) $(CPPFLAGS) $(INCLUDE) -o $@ -c $<

#$(OUTPUT_DIR)/%.o: %.cpp
#	$(ECHO) Compiling $<...
#	$(CXX) $(CPPFLAGS) $(INCLUDE) -o $@ -c $<

#$(OUTPUT_DIR)/%.o: %.cxx
#	$(ECHO) Compiling $<...
#	$(CXX) $(CPPFLAGS) $(INCLUDE) -o $@ -c $<

$(OUTPUT_DIR)/%.res: %.rc
	$(ECHO) Compiling $<...
	$(SILENT)$(RC) $(CPPFLAGS) $(INCLUDE) -o $@ -i $<

# WebASssembly/emscripten target
#
# See webassembly/WebAssembly.md
wasm:
	emmake make

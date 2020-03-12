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

#
# Flags
#
#CFLAGS   = -w -O3 -DNO_ASM -DNO_HIRES_TIMERS -DNO_FILE_HISTORY -DNO_EMBEDDED_SAMPLES -DUSE_SDL
#CPPFLAGS = -O3 -DNO_ASM

# debugger support
CFLAGS   = -g -w -DDEBUG_ENABLED -DLSB_FIRST -DNO_ASM -DNO_FILE_HISTORY -DNO_EMBEDDED_SAMPLES -DUSE_SDL -Wall -Werror

# production flags
#CFLAGS   = -g -w -O2 -DLSB_FIRST -DNO_FILE_HISTORY -DNO_EMBEDDED_SAMPLES -DUSE_SDL -Wall -Werror

#CFLAGS   += -DSINGLE_THREADED -DNO_TIMERS
#CFLAGS   += -DNO_HIRES_TIMERS
#CFLAGS   += -DEMU_FREQUENCY=3579545
CFLAGS   += -DEMU_FREQUENCY=8000000
#CFLAGS   += -DTRACE

#CPPFLAGS = -g -DNO_ASM
#

# ym3812 opl sound
CPPFLAGS +=-DBUILD_YM3812

LIBS     = -lSDL -lm#-lz -lGL
TARGET   = steckschwein-emu

SRCS        = $(SOURCE_FILES)
OBJS        = $(patsubst %.rc,%.res,$(patsubst %.cxx,%.o,$(patsubst %.cpp,%.o,$(patsubst %.cc,%.o,$(patsubst %.c,%.o,$(filter %.c %.cc %.cpp %.cxx %.rc,$(SRCS)))))))
OUTPUT_OBJS = $(addprefix $(OUTPUT_DIR)/, $(OBJS))

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
SDL_CFLAGS := $(shell sdl-config $(SDL_PREFIX) --cflags)
SDL_LDFLAGS := $(shell sdl-config $(SDL_PREFIX) --libs)
CFLAGS += $(SDL_CFLAGS)
CPPFLAGS += $(SDL_CFLAGS)
LDFLAGS += $(SDL_LDFLAGS)

ifdef CROSS_COMPILE_WINDOWS
	CFLAGS+=-Wno-error=deprecated-declarations
#	CFLAGS+=-Wno-error=incompatible-pointer-types
	CFLAGS+=-Wno-error=maybe-uninitialized
#	CFLAGS+=-DENABLE_VRAM_DECAY \
	CFLAGS+=-D_REENTRAN
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

ifdef EMSCRIPTEN
	LDFLAGS+=--shell-file webassembly/steckschwein-emu-template.html
	#LDFLAGS+=--preload-file rom.bin
 	#LDFLAGS+=-s DISABLE_DEPRECATED_FIND_EVENT_TARGET_BEHAVIOR=1
	# To the Javascript runtime exported functions
	LDFLAGS+=-s EXPORTED_FUNCTIONS='["_j2c_reset", "_j2c_paste", "_j2c_start_audio", _main]' -s EXTRA_EXPORTED_RUNTIME_METHODS='["ccall", "cwrap"]'

	OUTPUT=steckschwein-emu.html
endif

ifneq ("$(wildcard ./rom_labels.h)","")
HEADERS+=rom_labels.h
endif

#
# Tools
#
CC    = $(SILENT)gcc
CXX   = $(SILENT)g++
LD    = $(SILENT)g++
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
INCLUDE =
INCLUDE += -I$(ROOT_DIR)
INCLUDE += -I$(ROOT_DIR)/extern/include
INCLUDE += -I$(ROOT_DIR)/extern/src
# blue msx include stuff
INCLUDE += -I$(EXTERN_BLUEMSX_DIR)/Src/Arch
INCLUDE += -I$(EXTERN_BLUEMSX_DIR)/Src/Board
INCLUDE += -I$(EXTERN_BLUEMSX_DIR)/Src/Common
INCLUDE += -I$(EXTERN_BLUEMSX_DIR)/Src/Cpu
INCLUDE += -I$(EXTERN_BLUEMSX_DIR)/Src/Debugger
INCLUDE += -I$(EXTERN_BLUEMSX_DIR)/Src/Emulator
INCLUDE += -I$(EXTERN_BLUEMSX_DIR)/Src/Memory
INCLUDE += -I$(EXTERN_BLUEMSX_DIR)/Src/Sdl
INCLUDE += -I$(EXTERN_BLUEMSX_DIR)/Src/SoundChips
INCLUDE += -I$(EXTERN_BLUEMSX_DIR)/Src/VideoChips
INCLUDE += -I$(EXTERN_BLUEMSX_DIR)/Src/VideoRender

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
SOURCE_FILES += Properties.c
SOURCE_FILES += Board.c
SOURCE_FILES += IoPort.c
SOURCE_FILES += Steckschwein.c
SOURCE_FILES += MOS6502.c
SOURCE_FILES += DebugDeviceManager.c
SOURCE_FILES += Debugger.c

# wasm
SOURCE_FILES += javascript_interface.c

all: $(OUTPUT_DIR) $(TARGET)

$(TARGET): $(OUTPUT_OBJS)
	$(ECHO) Linking $@... $(LD) $(LDFLAGS) -o $@ $(OUTPUT_OBJS) $(LIBS)
	$(LD) $(LDFLAGS) -o $@ $(OUTPUT_OBJS) $(LIBS)

clean: clean_$(TARGET)

clean_$(TARGET):
	$(ECHO) Cleaning files ...
	$(RMDIR) -rf $(OUTPUT_DIR)
	$(RM) -f $(TARGET)
#	rm -f *.o cpu/*.o extern/src/*.o steckschwein-emu steckschwein-emu.exe steckschwein-emu.js steckschwein-emu.wasm steckschwein-emu.data steckschwein-emu.worker.js steckschwein-emu.html steckschwein-emu.html.mem

cpu/tables.h cpu/mnemonics.h: cpu/buildtables.py cpu/6502.opcodes cpu/65c02.opcodes
	cd cpu && python buildtables.py

$(OUTPUT_DIR):
	$(ECHO) Creating directory $@...
	$(MKDIR) $(OUTPUT_DIR)

$(OUTPUT_DIR)/%.o: %.c  $(HEADER_FILES)
	$(ECHO) Compiling $<...
	$(CC) $(CFLAGS) $(CPPFLAGS) $(INCLUDE) -o $@ -c $<

$(OUTPUT_DIR)/%.o: %.cc  $(HEADER_FILES)
	$(ECHO) Compiling $<...
	$(CXX) $(CPPFLAGS) $(INCLUDE) -o $@ -c $<

$(OUTPUT_DIR)/%.o: %.cpp  $(HEADER_FILES)
	$(ECHO) Compiling $<...
	$(CXX) $(CPPFLAGS) $(INCLUDE) -o $@ -c $<

$(OUTPUT_DIR)/%.o: %.cxx  $(HEADER_FILES)
	$(ECHO) Compiling $<...
	$(CXX) $(CPPFLAGS) $(INCLUDE) -o $@ -c $<

$(OUTPUT_DIR)/%.res: %.rc $(HEADER_FILES)
	$(ECHO) Compiling $<...
	$(RC) $(CPPFLAGS) $(INCLUDE) -o $@ -i $<

# WebASssembly/emscripten target
#
# See webassembly/WebAssembly.md
wasm:
	emmake make

#
# windows 10 build
#   CROSS_COMPILE_WINDOWS=1 WIN_SDL2=<sdl2 home> CC=<gcc home>/mingw32-gcc make clean all
#
ifndef (MINGW32)
	# the mingw32 path on macOS installed through homebrew
	MINGW32=/usr/local/Cellar/mingw-w64/6.0.0_2/toolchain-i686/i686-w64-mingw32
endif

# the Windows SDL2 path on macOS installed through ./configure --prefix=... && make && make install
ifndef WIN_SDL
	WIN_SDL=~/tmp/sdl2-win32
endif

CFLAGS=-O3 -Wall -Werror -g $(shell sdl-config --prefix=$(WIN_SDL) --cflags) -Iextern/include -Iextern/src
LDFLAGS=$(shell sdl-config --prefix=$(WIN_SDL) --libs) -lm

ifeq ($(CROSS_COMPILE_WINDOWS),1)
	CFLAGS+=-Wno-error=deprecated-declarations
#	CFLAGS+=-Wno-error=incompatible-pointer-types
	CFLAGS+=-Wno-error=maybe-uninitialized
#	CFLAGS+=-DENABLE_VRAM_DECAY \
	CFLAGS+=-D_REENTRAN
else
	CFLAGS+=-std=c99
endif

OUTPUT=steckschwein-emu

ifeq ($(MAC_STATIC),1)
	LDFLAGS=/usr/local/lib/libSDL2.a -lm -liconv -Wl,-framework,CoreAudio -Wl,-framework,AudioToolbox -Wl,-framework,ForceFeedback -lobjc -Wl,-framework,CoreVideo -Wl,-framework,Cocoa -Wl,-framework,Carbon -Wl,-framework,IOKit -Wl,-weak_framework,QuartzCore -Wl,-weak_framework,Metal
endif

ifeq ($(CROSS_COMPILE_WINDOWS),1)
	LDFLAGS+=-L$(MINGW32)/lib
	# this enables printf() to show, but also forces a console window
	LDFLAGS+=-mconsole -Wl,--subsystem,console
endif

ifdef EMSCRIPTEN
	LDFLAGS+=--shell-file webassembly/steckschwein-emu-template.html --preload-file rom.bin -s DISABLE_DEPRECATED_FIND_EVENT_TARGET_BEHAVIOR=1
	# To the Javascript runtime exported functions
	LDFLAGS+=-s EXPORTED_FUNCTIONS='["_j2c_reset", "_j2c_paste", "_j2c_start_audio", _main]' -s EXTRA_EXPORTED_RUNTIME_METHODS='["ccall", "cwrap"]'

	OUTPUT=steckschwein-emu.html
endif

OBJS = cpu/fake6502.o memory.o disasm.o via.o uart.o vdp_adapter.o opl2.o spi.o sdcard.o main.o javascript_interface.o # rendertext.o debugger.o 

HEADERS = disasm.h cpu/fake6502.h glue.h memory.h vdp_adapter.h via.h

ifneq ("$(wildcard ./rom_labels.h)","")
HEADERS+=rom_labels.h
endif


all: $(OBJS) $(HEADERS)
	$(CC) -o $(OUTPUT) $(OBJS) $(LDFLAGS)
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

cpu/tables.h cpu/mnemonics.h: cpu/buildtables.py cpu/6502.opcodes cpu/65c02.opcodes
	cd cpu && python buildtables.py


# WebASssembly/emscripten target
#
# See webassembly/WebAssembly.md
wasm:
	emmake make

clean:
	rm -f *.o cpu/*.o extern/src/*.o steckschwein-emu steckschwein-emu.exe steckschwein-emu.js steckschwein-emu.wasm steckschwein-emu.data steckschwein-emu.worker.js steckschwein-emu.html steckschwein-emu.html.mem

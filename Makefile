
# the mingw32 path on macOS installed through homebrew
MINGW32=/usr/local/Cellar/mingw-w64/6.0.0_2/toolchain-i686/i686-w64-mingw32
# the Windows SDL2 path on macOS installed through ./configure --prefix=... && make && make install
WIN_SDL2=~/tmp/sdl2-win32

ifeq ($(CROSS_COMPILE_WINDOWS),1)
	SDL2CONFIG=$(WIN_SDL2)/bin/sdl2-config
else
	SDL2CONFIG=sdl2-config
endif

CFLAGS=-std=c99 -O3 -Wall -Werror -g $(shell $(SDL2CONFIG) --cflags) -Iextern/include -Iextern/src
LDFLAGS=$(shell $(SDL2CONFIG) --libs) -lm

OUTPUT=steckschwein-emu

ifeq ($(MAC_STATIC),1)
	LDFLAGS=/usr/local/lib/libSDL2.a -lm -liconv -Wl,-framework,CoreAudio -Wl,-framework,AudioToolbox -Wl,-framework,ForceFeedback -lobjc -Wl,-framework,CoreVideo -Wl,-framework,Cocoa -Wl,-framework,Carbon -Wl,-framework,IOKit -Wl,-weak_framework,QuartzCore -Wl,-weak_framework,Metal
endif

ifeq ($(CROSS_COMPILE_WINDOWS),1)
	LDFLAGS+=-L$(MINGW32)/lib
	# this enables printf() to show, but also forces a console window
	LDFLAGS+=-Wl,--subsystem,console
	CC=i686-w64-mingw32-gcc
endif

ifdef EMSCRIPTEN
	LDFLAGS+=--shell-file webassembly/steckschwein-emu-template.html --preload-file rom.bin -s DISABLE_DEPRECATED_FIND_EVENT_TARGET_BEHAVIOR=1
	# To the Javascript runtime exported functions
	LDFLAGS+=-s EXPORTED_FUNCTIONS='["_j2c_reset", "_j2c_paste", "_j2c_start_audio", _main]' -s EXTRA_EXPORTED_RUNTIME_METHODS='["ccall", "cwrap"]'

	OUTPUT=steckschwein-emu.html
endif

OBJS = cpu/fake6502.o memory.o disasm.o video.o via.o uart.o vdp.o opl2.o spi.o sdcard.o main.o debugger.o javascript_interface.o rendertext.o # v9938.o

HEADERS = disasm.h cpu/fake6502.h glue.h memory.h video.h via.h

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

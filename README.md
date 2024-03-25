Steckschwein Emulator
======================

This is the emulator for the 8bit Steckschwein homebrew computer forked from the X16 naked Emulator and adapted to blueMSX. It only depends on SDL and therefore can be compiled on all modern operating systems.


Binaries & Compiling
--------------------

<!--a href="https://travis-ci.org/commanderx16/x16-emulator"><img alt="Travis (.org)" src="https://img.shields.io/travis/commanderx16/x16-emulator.svg?label=CI&logo=travis&logoColor=white&style=for-the-badge"></a-->

Binary releases for MacOS, Windows and x86_64 Linux are available on the [releases page][releases].

The emulator itself is dependent only on SDL. However, to run the emulated system you will also need a compatible `rom.bin` ROM image. This will be
loaded from the directory containing the emulator binary, or you can use the `-rom .../path/to/rom.bin` option.

> __WARNING:__ Older versions of the ROM might not work in newer versions of the emulator, and vice versa.

You can build a Steckschwein ROM image yourself using the [build instructions][x16rom-build] in the [Steckschwein Repository][steckschwein-repo]. The `rom.bin` included in the [_latest_ release][releases] of the emulator may also work with the HEAD of this repo, but this is not guaranteed.

### Linux Build

The SDL Version 2 development package is available as a distribution package with most major versions of Linux:
- Red Hat: `yum install SDL-devel`
- Debian: `apt-get install libsdl2-dev`

Type `make` to build the source. The output will be `steckschwein-emu` in the current directory. Remember you will also need a `rom.bin` as described above.

### WebAssembly Build

Steps for compiling WebAssembly/HTML5 can be found [here][webassembly].


Starting
--------

You can start `steckschwein-emu`/`steckschwein-emu.exe` either by double-clicking it, or from the command line. The latter allows you to specify additional arguments.

* When starting `steckschwein-emu` without arguments, it will pick up the system ROM (`rom.bin`) from the executable's directory.
* The system ROM filename/path can be overridden with the `-rom` command line argument.
* `-keymap` tells the KERNAL to switch to a specific keyboard layout. Use it without an argument to view the supported layouts.
* `-sdcard` lets you specify an SD card image (partition table + FAT32).
* `-prg` lets you specify a `.prg` file that gets injected into RAM after start.
* `-bas` lets you specify a BASIC program in ASCII format that automatically typed in (and tokenized).
* `-run` executes the application specified through `-prg` or `-bas` using `RUN` or `SYS`, depending on the load address.
* `-scale` scales video output to an integer multiple of 640x480
* `-echo` causes all KERNAL/BASIC output to be printed to the host's terminal. Enable this and use the BASIC command "LIST" to convert a BASIC program to ASCII (detokenize).
* `-gif <filename>[,wait]` to record the screen into a GIF. See below for more info.
* `-scale` scales video output to an integer multiple of 640x480
* `-quality` change image scaling algorithm quality
	* `nearest`: nearest pixel sampling
	* `linear`: linear filtering
	* `best`: (default) anisotropic filtering
* `-log` enables one or more types of logging (e.g. `-log KS`):
	* `K`: keyboard (key-up and key-down events)
	* `S`: speed (CPU load, frame misses)
	* `V`: video I/O reads and writes
* `-debug` enables the debugger.
* `-dump` configure system dump (e.g. `-dump CB`):
	* `C`: CPU registers (7 B: A,X,Y,SP,STATUS,PC)
	* `R`: RAM (40 KiB)
	* `B`: Banked RAM (2 MiB)
	* `V`: Video RAM and registers (128 KiB VRAM, 32 B composer registers, 512 B pallete, 16 B layer0 registers, 16 B layer1 registers, 16 B sprite registers, 2 KiB sprite attributes)
* When compiled with `WITH_YM2151`, `-sound` can be used to specify the output sound device.
* When compiled with `#define TRACE`, `-trace` will enable an instruction trace on stdout.

Run `steckschwein-emu -h` to see all command line options.


Keyboard Layout
---------------

The X16 uses a PS/2 keyboard, and the ROM currently supports several different layouts. The following table shows their names, and what keys produce different characters than expected:

|Name  |Description 	       |Differences|
|------|------------------------|-------|
|en-us |US		       |[`] ⇒ [←], [~] ⇒ [π], [&#92;] ⇒ [£]|
|en-gb |United Kingdom	       |[`] ⇒ [←], [~] ⇒ [π]|
|de    |German		       |[§] ⇒ [£], [´] ⇒ [^], [^] ⇒ [←], [°] ⇒ [π]|
|nordic|Nordic                 |key left of [1] ⇒ [←],[π]|
|it    |Italian		       |[&#92;] ⇒ [←], [&vert;] ⇒ [π]|
|pl    |Polish (Programmers)   |[`] ⇒ [←], [~] ⇒ [π], [&#92;] ⇒ [£]|
|hu    |Hungarian	       |[&#92;] ⇒ [←], [&vert;] ⇒ [π], [§] ⇒ [£]|
|es    |Spanish		       |[&vert;] ⇒ π, &#92; ⇒ [←], Alt + [<] ⇒ [£]|
|fr    |French		       |[²] ⇒ [←], [§] ⇒ [£]|
|de-ch |Swiss German	       |[^] ⇒ [←], [°] ⇒ [π]|
|fr-be |Belgian French	       |[²] ⇒ [←], [³] ⇒ [π]|
|fi    |Finnish		       |[§] ⇒ [←], [½] ⇒ [π]|
|pt-br |Portuguese (Brazil ABNT)|[&#92;] ⇒ [←], [&vert;] ⇒ [π]|

Keys that produce international characters (like [ä] or [ç]) will not produce any character.

Since the emulator tells the computer the *position* of keys that are pressed, you need to configure the layout for the computer independently of the keyboard layout you have configured on the host.

**Use the F9 key to cycle through the layouts, or set the keyboard layout at startup using the `-keymap` command line argument.**

The following keys can be used for controlling games:

|Keyboard Key  | NES Equivalent |
|--------------|----------------|
|Ctrl          | A 		|
|Alt 	       | B		|
|Space         | SELECT         |
|Enter         | START		|
|Cursor Up     | UP		|
|Cursor Down   | DOWN		|
|Cursor Left   | LEFT		|
|Cursor Right  | RIGHT		|


Functions while running
-----------------------

* Ctrl + R will reset the computer.
* Ctrl + V will paste the clipboard by injecting key presses.
* Ctrl + S will save a system dump (configurable with `-dump`) to disk.
* Ctrl + Return will toggle full screen mode.

On the Mac, use the Cmd key instead.


GIF Recording
-------------

With the argument `-gif`, followed by a filename, a screen recording will be saved into the given GIF file. Please exit the emulator before reading the GIF file.

If the option `,wait` is specified after the filename, it will start recording on `POKE $9FB5,2`. It will capture a single frame on `POKE $9FB5,1` and pause recording on `POKE $9FB5,0`. `PEEK($9FB5)` returns a 128 if recording is enabled but not active.


BASIC and the Screen Editor
---------------------------

On startup, the X16 presents direct mode of BASIC V2. You can enter BASIC statements, or line numbers with BASIC statements and `RUN` the program, just like on Commodore computers.

* To stop execution of a BASIC program, hit the RUN/STOP key (Esc in the emulator), or Ctrl + C.
* To insert a character, press Shift + Backspace.
* To clear the screen, press Shift + Home.
* The X16 does not have a STOP+RESTORE function.


Host Filesystem Interface
-------------------------

If the system ROM contains any version of the KERNAL, the LOAD (`$FFD5`) and SAVE (`$FFD8`) KERNAL calls are intercepted by the emulator if the device is 1 (which is the default). So the BASIC statements

      LOAD"$
      LOAD"FOO.PRG
      LOAD"IMAGE.PRG",1,1
      SAVE"BAR.PRG

will target the host computer's local filesystem.

The emulator will interpret filesnames relative to the directory it was started in. Note that on macOS, when double-clicking the executable, this is the home directory.

To avoid incompatibility problems between the PETSCII and ASCII encodings, use lower case filenames on the host side, and unshifted filenames on the X16 side.


Dealing with BASIC Programs
---------------------------

BASIC programs are encoded in a tokenized form, they are not simply ASCII files. If you want to edit BASIC programs on the host's text editor, you need to convert it between tokenized BASIC form and ASCII.

* To convert ASCII to BASIC, reboot the machine and paste the ASCII text using Ctrl + V (Mac: Cmd + V). You can now run the program, or use the `SAVE` BASIC command to write the tokenized version to disk.
* To convert BASIC to ASCII, start steckschwein-emu with the -echo argument, `LOAD` the BASIC file, and type `LIST`. Now copy the ASCII version from the terminal.


Using the KERNAL/BASIC environment
----------------------------------

Please see the KERNAL/BASIC documentation.


Debugger
--------

The debugger requires `-debug` to start. Without it it is effectively disabled.

There are 2 panels you can control. The code panel, the top left half, and the data panel, the bottom half of the screen. The displayed address can be changed using keys 0-9 and A-F, in a 'shift and roll' manner (easier to see than explain). To change the data panel address, press the shift key and type 0-9 A-F. The top write panel is fixed.

The debugger keys are similar to the Microsoft Debugger shortcut keys, and work as follows

|Key|Description 																			|
|---|---------------------------------------------------------------------------------------|
|F1 |resets the shown code position to the current PC										|
|F2 |resets the 65C02 CPU but not any of the hardware.										|
|F5 |is used to return to Run mode, the emulator should run as normal.						|
|F9 |sets the breakpoint to the currently code position.									|
|F10|steps 'over' routines - if the next instruction is JSR it will break on return.		|
|F11|steps 'into' routines.																	|
|F12|is used to break back into the debugger. This does not happen if you do not have -debug|
|TAB|when stopped, or single stepping, hides the debug information when pressed 			|

When -debug is selected the 65c02 STP Opcode ($DB) will break into the debugger automatically.

Effectively keyboard routines only work when the debugger is running normally. Single stepping through keyboard code will not work at present.


Wiki
----

https://github.com/commanderx16/x16-emulator/wiki


Features
--------

* CPU: Full 65C02 instruction set (improved "fake6502")
* MSX Video (TMS9929, VDP9938, VDP9958)
	* Mostly cycle exact emulation
	* Supports almost all features: composer, two layers, sprites, progressive/interlaced
* VIA
	* ROM/RAM banking
	* PS/2 keyboard
	* SD card (SPI)
* Sound (YM3812)


Missing Features
----------------

* VIA
	* Does not support counters/timers/IRQs
	* Does not support game controllers

License
-------

Copyright (c) 2019 Michael Steil &lt;mist64@mac.com&gt;, [www.pagetable.com](https://www.pagetable.com/).
All rights reserved. License: 2-clause BSD


Known Issues
------------

* Emulator: LOAD"$ (and LOAD"$",1) will show host uppercase filenames as garbage. Use Ctrl+N to switch to the X16 upper/lower character set for a workaround.


Release Notes
-------------
- 0.1 ?

<!-------------------------------------------------------------------->
[releases]: https://github.com/commanderx16/x16-emulator/releases
[webassembly]: webassembly/WebAssembly.md
[x16rom-build]: https://github.com/commanderx16/x16-rom#releases-and-building
[steckschwein-repo]: https://bitbucket.org/steckschwein/steckschwein-code/src/default/steckos/bios/


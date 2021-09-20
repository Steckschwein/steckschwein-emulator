// *******************************************************************************************
// *******************************************************************************************
//
//		File:		65C02.H
//		Date:		3rd September 2019
//		Purpose:	Additional functions for new 65C02 Opcodes.
//		Author:		Paul Robson (paul@robson.org.uk)
//
// *******************************************************************************************
// *******************************************************************************************
// *******************************************************************************************
//
//					Indirect without indexation.  (copied from indy)
//
// *******************************************************************************************

static void ind0() {
    uint16_t eahelp, eahelp2;
    eahelp = (uint16_t)read6502(pc++);
    eahelp2 = (eahelp & 0xFF00) | ((eahelp + 1) & 0x00FF); //zero-page wraparound
    ea = (uint16_t)read6502(eahelp) | ((uint16_t)read6502(eahelp2) << 8);
}


// *******************************************************************************************
//
//						(Absolute,Indexed) address mode for JMP
//
// *******************************************************************************************

static void ainx() { 		// absolute indexed branch
    uint16_t eahelp, eahelp2;
    eahelp = (uint16_t)read6502(pc) | (uint16_t)((uint16_t)read6502(pc+1) << 8);
    eahelp = (eahelp + (uint16_t)x) & 0xFFFF;
    eahelp2 = (eahelp & 0xFF00) | ((eahelp + 1) & 0x00FF); //replicate 6502 page-boundary wraparound bug
    ea = (uint16_t)read6502(eahelp) | ((uint16_t)read6502(eahelp2) << 8);
    pc += 2;
}

// *******************************************************************************************
//
//								Store zero to memory.
//
// *******************************************************************************************

static void stz() {
    putvalue(0);
}

// *******************************************************************************************
//
//								Unconditional Branch
//
// *******************************************************************************************

static void bra() {
    oldpc = pc;
    pc += reladdr;
    if ((oldpc & 0xFF00) != (pc & 0xFF00)) clockticks6502 += 2; //check if jump crossed a page boundary
        else clockticks6502++;
}

// *******************************************************************************************
//
//									Push/Pull X and Y
//
// *******************************************************************************************

static void phx() {
    push8(x);
}

static void plx() {
    x = pull8();

    zerocalc(x);
    signcalc(x);
}

static void phy() {
    push8(y);
}

static void ply() {
    y = pull8();

    zerocalc(y);
    signcalc(y);
}

// *******************************************************************************************
//
//								TRB & TSB - Test and Change bits
//
// *******************************************************************************************

static void tsb() {
    value = getvalue(); 							// Read memory
    result = (uint16_t)a & value;  					// calculate A & memory
    zerocalc(result); 								// Set Z flag from this.
    result = value | a; 							// Write back value read, A bits are set.
    putvalue(result);
}

static void trb() {
    value = getvalue(); 							// Read memory
    result = (uint16_t)a & value;  					// calculate A & memory
    zerocalc(result); 								// Set Z flag from this.
    result = value & (a ^ 0xFF); 					// Write back value read, A bits are clear.
    putvalue(result);
}

static void rmb0() {
    value = getvalue();
    result = (uint16_t) value & 0b11111110;
    putvalue(result);
}
static void rmb1() {
    value = getvalue();
    result = (uint16_t) value & 0b11111101;
    putvalue(result);
}
static void rmb2() {
    value = getvalue();
    result = (uint16_t) value & 0b11111011;
    putvalue(result);
}
static void rmb3() {
    value = getvalue();
    result = (uint16_t) value & 0b11110111;
    putvalue(result);
}
static void rmb4() {
    value = getvalue();
    result = (uint16_t) value & 0b11101111;
    putvalue(result);
}
static void rmb5() {
    value = getvalue();
    result = (uint16_t) value & 0b11011111;
    putvalue(result);
}
static void rmb6() {
    value = getvalue();
    result = (uint16_t) value & 0b10111111;
    putvalue(result);
}
static void rmb7() {
    value = getvalue();
    result = (uint16_t) value & 0b01111111;
    putvalue(result);
}

static void smb0() {
    value = getvalue();
    result = (uint16_t) value ^ 0b00000001;
    putvalue(result);
}
static void smb1() {
    value = getvalue();
    result = (uint16_t) value ^ 0b00000010;
    putvalue(result);
}
static void smb2() {
    value = getvalue();
    result = (uint16_t) value ^ 0b00000100;
    putvalue(result);
}
static void smb3() {
    value = getvalue();
    result = (uint16_t) value ^ 0b00001000;
    putvalue(result);
}
static void smb4() {
    value = getvalue();
    result = (uint16_t) value ^ 0b00010000;
    putvalue(result);
}
static void smb5() {
    value = getvalue();
    result = (uint16_t) value ^ 0b00100000;
    putvalue(result);
}
static void smb6() {
    value = getvalue();
    result = (uint16_t) value ^ 0b01000000;
    putvalue(result);
}
static void smb7() {
    value = getvalue();
    result = (uint16_t) value ^ 0b10000000;
    putvalue(result);
}

static void bbsr_X(uint8_t bit, uint8_t predicate) {

    value = getvalue();
    if ((value & (1<<bit)) == (predicate<<bit)) {
	    oldpc = pc;
	    pc += reladdr;
	    if ((oldpc & 0xFF00) != (pc & 0xFF00)) clockticks6502++; //if branch crossed a page boundary, +1 cl
    }
}

static void bbr0() { bbsr_X(0, 0); }
static void bbr1() { bbsr_X(1, 0); }
static void bbr2() { bbsr_X(2, 0); }
static void bbr3() { bbsr_X(3, 0); }
static void bbr4() { bbsr_X(4, 0); }
static void bbr5() { bbsr_X(5, 0); }
static void bbr6() { bbsr_X(6, 0); }
static void bbr7() { bbsr_X(7, 0); }

static void bbs0() { bbsr_X(0, 1); }
static void bbs1() { bbsr_X(1, 1); }
static void bbs2() { bbsr_X(2, 1); }
static void bbs3() { bbsr_X(3, 1); }
static void bbs4() { bbsr_X(4, 1); }
static void bbs5() { bbsr_X(5, 1); }
static void bbs6() { bbsr_X(6, 1); }
static void bbs7() { bbsr_X(7, 1); }

// *******************************************************************************************
//
//                                     Invoke Debugger
//
// *******************************************************************************************
#include "EmulatorDebugger.h"
static void dbg() {
	DEBUGBreakToDebugger();                          // Invoke debugger.
}

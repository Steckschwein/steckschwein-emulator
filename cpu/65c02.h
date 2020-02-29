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


static void bbs7() {
    value = getvalue();
    if (value & 0b10000000) {
    
	    oldpc = pc;
	    pc += reladdr;
	    if ((oldpc & 0xFF00) != (pc & 0xFF00)) clockticks6502 += 2; //check if jump crossed a page boundary
		else clockticks6502++;
    }   
}

// *******************************************************************************************
//
//                                     Invoke Debugger
//
// *******************************************************************************************

static void dbg() {
    DEBUGBreakToDebugger();                          // Invoke debugger.
}

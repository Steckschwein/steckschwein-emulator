/* Generated by buildtables.py */

static void (*addrtable[256])() = {
/*        |  0  |  1  |  2  |  3  |  4  |  5  |  6  |  7  |  8  |  9  |  A  |  B  |  C  |  D  |  E  |  F  |     */
/* 0 */     imp, indx,  imp,  imp,   zp,   zp,   zp,   zp,  imp,  imm,  acc,  imp, abso, abso, abso, zprel, /* 0 */
/* 1 */     rel, indy, ind0,  imp,   zp,  zpx,  zpx,   zp,  imp, absy,  acc,  imp, abso, absx, absx, zprel, /* 1 */
/* 2 */    abso, indx,  imp,  imp,   zp,   zp,   zp,   zp,  imp,  imm,  acc,  imp, abso, abso, abso, zprel, /* 2 */
/* 3 */     rel, indy, ind0,  imp,  zpx,  zpx,  zpx,   zp,  imp, absy,  acc,  imp, absx, absx, absx, zprel, /* 3 */
/* 4 */     imp, indx,  imp,  imp,  imp,   zp,   zp,   zp,  imp,  imm,  acc,  imp, abso, abso, abso, zprel, /* 4 */
/* 5 */     rel, indy, ind0,  imp,  imp,  zpx,  zpx,   zp,  imp, absy,  imp,  imp,  imp, absx, absx, zprel, /* 5 */
/* 6 */     imp, indx,  imp,  imp,   zp,   zp,   zp,   zp,  imp,  imm,  acc,  imp,  ind, abso, abso, zprel, /* 6 */
/* 7 */     rel, indy, ind0,  imp,  zpx,  zpx,  zpx,   zp,  imp, absy,  imp,  imp, ainx, absx, absx, zprel, /* 7 */
/* 8 */     rel, indx,  imp,  imp,   zp,   zp,   zp,   zp,  imp,  imm,  imp,  imp, abso, abso, abso, zprel, /* 8 */
/* 9 */     rel, indy, ind0,  imp,  zpx,  zpx,  zpy,   zp,  imp, absy,  imp,  imp, abso, absx, absx, zprel, /* 9 */
/* A */     imm, indx,  imm,  imp,   zp,   zp,   zp,   zp,  imp,  imm,  imp,  imp, abso, abso, abso, zprel, /* A */
/* B */     rel, indy, ind0,  imp,  zpx,  zpx,  zpy,   zp,  imp, absy,  imp,  imp, absx, absx, absy, zprel, /* B */
/* C */     imm, indx,  imp,  imp,   zp,   zp,   zp,   zp,  imp,  imm,  imp,  imp, abso, abso, abso, zprel, /* C */
/* D */     rel, indy, ind0,  imp,  imp,  zpx,  zpx,   zp,  imp, absy,  imp,  imp,  imp, absx, absx, zprel, /* D */
/* E */     imm, indx,  imp,  imp,   zp,   zp,   zp,   zp,  imp,  imm,  imp,  imp, abso, abso, abso, zprel, /* E */
/* F */     rel, indy, ind0,  imp,  imp,  zpx,  zpx,   zp,  imp, absy,  imp,  imp,  imp, absx, absx, zprel  /* F */
};

static void (*optable[256])() = {
/*        |  0  |  1  |  2  |  3  |  4  |  5  |  6  |  7  |  8  |  9  |  A  |  B  |  C  |  D  |  E  |  F  |     */
/* 0 */      brk,  ora,  nop,  nop,  tsb,  ora,  asl, rmb0,  php,  ora,  asl,  nop,  tsb,  ora,  asl,  bbr0, /* 0 */
/* 1 */      bpl,  ora,  ora,  nop,  trb,  ora,  asl, rmb1,  clc,  ora,  inc,  nop,  trb,  ora,  asl,  bbr1, /* 1 */
/* 2 */      jsr,  and,  nop,  nop,  bit,  and,  rol, rmb2,  plp,  and,  rol,  nop,  bit,  and,  rol,  bbr2, /* 2 */
/* 3 */      bmi,  and,  and,  nop,  bit,  and,  rol, rmb3,  sec,  and,  dec,  nop,  bit,  and,  rol,  bbr3, /* 3 */
/* 4 */      rti,  eor,  nop,  nop,  nop,  eor,  lsr, rmb4,  pha,  eor,  lsr,  nop,  jmp,  eor,  lsr,  bbr4, /* 4 */
/* 5 */      bvc,  eor,  eor,  nop,  nop,  eor,  lsr, rmb5,  cli,  eor,  phy,  nop,  nop,  eor,  lsr,  bbr5, /* 5 */
/* 6 */      rts,  adc,  nop,  nop,  stz,  adc,  ror, rmb6,  pla,  adc,  ror,  nop,  jmp,  adc,  ror,  bbr6, /* 6 */
/* 7 */      bvs,  adc,  adc,  nop,  stz,  adc,  ror, rmb7,  sei,  adc,  ply,  nop,  jmp,  adc,  ror,  bbr7, /* 7 */
/* 8 */      bra,  sta,  nop,  nop,  sty,  sta,  stx, smb0,  dey,  bit,  txa,  nop,  sty,  sta,  stx,  bbs0, /* 8 */
/* 9 */      bcc,  sta,  sta,  nop,  sty,  sta,  stx, smb1,  tya,  sta,  txs,  nop,  stz,  sta,  stz,  bbs1, /* 9 */
/* A */      ldy,  lda,  ldx,  nop,  ldy,  lda,  ldx, smb2,  tay,  lda,  tax,  nop,  ldy,  lda,  ldx,  bbs2, /* A */
/* B */      bcs,  lda,  lda,  nop,  ldy,  lda,  ldx, smb3,  clv,  lda,  tsx,  nop,  ldy,  lda,  ldx,  bbs3, /* B */
/* C */      cpy,  cmp,  nop,  nop,  cpy,  cmp,  dec, smb4,  iny,  cmp,  dex,  nop,  cpy,  cmp,  dec,  bbs4, /* C */
/* D */      bne,  cmp,  cmp,  nop,  nop,  cmp,  dec, smb5,  cld,  cmp,  phx,  stp,  nop,  cmp,  dec,  bbs5, /* D */
/* E */      cpx,  sbc,  nop,  nop,  cpx,  sbc,  inc, smb6,  inx,  sbc,  nop,  nop,  cpx,  sbc,  inc,  bbs6, /* E */
/* F */      beq,  sbc,  sbc,  nop,  nop,  sbc,  inc, smb7,  sed,  sbc,  plx,  nop,  nop,  sbc,  inc,  bbs7  /* F */
};

static const uint32_t ticktable[256] = {
/*        |  0  |  1  |  2  |  3  |  4  |  5  |  6  |  7  |  8  |  9  |  A  |  B  |  C  |  D  |  E  |  F  |     */
/* 0 */       7,    6,    2,    2,    5,    3,    5,    5,    3,    2,    2,    2,    6,    4,    6,    5, /* 0 */
/* 1 */       2,    5,    5,    2,    5,    4,    6,    5,    2,    4,    2,    2,    6,    4,    7,    5, /* 1 */
/* 2 */       6,    6,    2,    2,    3,    3,    5,    5,    4,    2,    2,    2,    4,    4,    6,    5, /* 2 */
/* 3 */       2,    5,    5,    2,    4,    4,    6,    5,    2,    4,    2,    2,    4,    4,    7,    5, /* 3 */
/* 4 */       6,    6,    2,    2,    2,    3,    5,    5,    3,    2,    2,    2,    3,    4,    6,    5, /* 4 */
/* 5 */       2,    5,    5,    2,    2,    4,    6,    5,    2,    4,    3,    2,    2,    4,    7,    5, /* 5 */
/* 6 */       6,    6,    2,    2,    3,    3,    5,    5,    4,    2,    2,    2,    5,    4,    6,    5, /* 6 */
/* 7 */       2,    5,    5,    2,    4,    4,    6,    5,    2,    4,    4,    2,    6,    4,    7,    5, /* 7 */
/* 8 */       3,    6,    2,    2,    3,    3,    3,    5,    2,    2,    2,    2,    4,    4,    4,    5, /* 8 */
/* 9 */       2,    6,    5,    2,    4,    4,    4,    5,    2,    5,    2,    2,    4,    5,    5,    5, /* 9 */
/* A */       2,    6,    2,    2,    3,    3,    3,    5,    2,    2,    2,    2,    4,    4,    4,    5, /* A */
/* B */       2,    5,    5,    2,    4,    4,    4,    5,    2,    4,    2,    2,    4,    4,    4,    5, /* B */
/* C */       2,    6,    2,    2,    3,    3,    5,    5,    2,    2,    2,    2,    4,    4,    6,    5, /* C */
/* D */       2,    5,    5,    2,    2,    4,    6,    5,    2,    4,    3,    1,    2,    4,    7,    5, /* D */
/* E */       2,    6,    2,    2,    3,    3,    5,    5,    2,    2,    2,    2,    4,    4,    6,    5, /* E */
/* F */       2,    5,    5,    2,    2,    4,    6,    5,    2,    4,    4,    2,    2,    4,    7,    5  /* F */
};

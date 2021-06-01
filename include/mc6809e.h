/*
 * mc6809e.h
 *
 * This header file lists MC6809E assembly op-codes,
 * command mnemonic, command cycle count, byte count, and
 * addressing mode.
 *
 * This static data structure is based on CPU data sheet
 * Motorola INC. 1984 DS9846-R2.
 *
 *  July 4, 2020
 *
 */

#ifndef __MC6809E_H__
#define __MC6809E_H__

#include    <stdint.h>

#define     ADDR_DIRECT             1
#define     ADDR_INHERENT           2
#define     ADDR_RELATIVE           3           // 8-bit address offset
#define     ADDR_LRELATIVE          4           // 16-bit address offset
#define     ADDR_INDEXED            5
#define     ADDR_EXTENDED           6
#define     ADDR_IMMEDIATE          7           // 8-bit immediate
#define     ADDR_LIMMEDIATE         8           // 16-bit immediate
#define     DOUBLE_BYTE             9           // Double byte commands starting with 0x10 or 0x10
#define     ILLEGAL_OP              10

#define     OP_CODE                 0
#define     OP_CODE10               256
#define     OP_CODE11               294

typedef struct
{
    int  op;
    char mnem[6];
    int  mode;
    int  cycles;
    int  bytes;
} machine_code_t;

machine_code_t machine_code[] = {
    {0x00, "neg"  , ADDR_DIRECT    , 6 , 2},
    {0x01, "???"  , ILLEGAL_OP     , 0 , 1},    // Zero cycles and "???" notes illegal op-code
    {0x02, "???"  , ILLEGAL_OP     , 0 , 1},
    {0x03, "com"  , ADDR_DIRECT    , 6 , 2},
    {0x04, "lsr"  , ADDR_DIRECT    , 6 , 2},
    {0x05, "???"  , ILLEGAL_OP     , 0 , 1},
    {0x06, "ror"  , ADDR_DIRECT    , 6 , 2},
    {0x07, "asr"  , ADDR_DIRECT    , 6 , 2},
    {0x08, "asl"  , ADDR_DIRECT    , 6 , 2},    // lsl
    {0x09, "rol"  , ADDR_DIRECT    , 6 , 2},
    {0x0a, "dec"  , ADDR_DIRECT    , 6 , 2},
    {0x0b, "???"  , ILLEGAL_OP     , 0 , 1},
    {0x0c, "inc"  , ADDR_DIRECT    , 6 , 2},
    {0x0d, "tst"  , ADDR_DIRECT    , 6 , 2},
    {0x0e, "jmp"  , ADDR_DIRECT    , 3 , 2},
    {0x0f, "clr"  , ADDR_DIRECT    , 6 , 2},

    {0x10, "0x10" , DOUBLE_BYTE    , 0 , 0},
    {0x11, "0x11" , DOUBLE_BYTE    , 0 , 0},
    {0x12, "nop"  , ADDR_INHERENT  , 2 , 1},
    {0x13, "sync" , ADDR_INHERENT  , 4 , 1},
    {0x14, "???"  , ILLEGAL_OP     , 0 , 1},
    {0x15, "???"  , ILLEGAL_OP     , 0 , 1},
    {0x16, "lbra" , ADDR_LRELATIVE , 5 , 3},
    {0x17, "lbsr" , ADDR_LRELATIVE , 9 , 3},
    {0x18, "???"  , ILLEGAL_OP     , 0 , 1},
    {0x19, "daa"  , ADDR_INHERENT  , 2 , 1},
    {0x1a, "orcc" , ADDR_IMMEDIATE , 3 , 2},
    {0x1b, "???"  , ILLEGAL_OP     , 0 , 1},
    {0x1c, "andcc", ADDR_IMMEDIATE , 3 , 2},
    {0x1d, "sex"  , ADDR_INHERENT  , 2 , 1},
    {0x1e, "exg"  , ADDR_IMMEDIATE , 8 , 2},
    {0x1f, "tfr"  , ADDR_IMMEDIATE , 6 , 2},

    {0x20, "bra"  , ADDR_RELATIVE  , 3 , 2},
    {0x21, "brn"  , ADDR_RELATIVE  , 3 , 2},
    {0x22, "bhi"  , ADDR_RELATIVE  , 3 , 2},
    {0x23, "bls"  , ADDR_RELATIVE  , 3 , 2},
    {0x24, "bcc"  , ADDR_RELATIVE  , 3 , 2},    // bhs
    {0x25, "bcs"  , ADDR_RELATIVE  , 3 , 2},    // blo
    {0x26, "bne"  , ADDR_RELATIVE  , 3 , 2},
    {0x27, "beq"  , ADDR_RELATIVE  , 3 , 2},
    {0x28, "bvc"  , ADDR_RELATIVE  , 3 , 2},
    {0x29, "bvs"  , ADDR_RELATIVE  , 3 , 2},
    {0x2a, "bpl"  , ADDR_RELATIVE  , 3 , 2},
    {0x2b, "bmi"  , ADDR_RELATIVE  , 3 , 2},
    {0x2c, "bge"  , ADDR_RELATIVE  , 3 , 2},
    {0x2d, "blt"  , ADDR_RELATIVE  , 3 , 2},
    {0x2e, "bgt"  , ADDR_RELATIVE  , 3 , 2},
    {0x2f, "ble"  , ADDR_RELATIVE  , 3 , 2},

    {0x30, "leax" , ADDR_INDEXED   , 4 , 2},
    {0x31, "leay" , ADDR_INDEXED   , 4 , 2},
    {0x32, "leas" , ADDR_INDEXED   , 4 , 2},
    {0x33, "leau" , ADDR_INDEXED   , 4 , 2},
    {0x34, "pshs" , ADDR_IMMEDIATE , 5 , 2},
    {0x35, "puls" , ADDR_IMMEDIATE , 5 , 2},
    {0x36, "pshu" , ADDR_IMMEDIATE , 5 , 2},
    {0x37, "pulu" , ADDR_IMMEDIATE , 5 , 2},
    {0x38, "???"  , ILLEGAL_OP     , 0 , 1},
    {0x39, "rts"  , ADDR_INHERENT  , 5 , 1},
    {0x3a, "abx"  , ADDR_INHERENT  , 3 , 1},
    {0x3b, "rti"  , ADDR_INHERENT  , 6 , 1},
    {0x3c, "cwai" , ADDR_INHERENT  , 20, 2},
    {0x3d, "mul"  , ADDR_INHERENT  , 11, 1},
    {0x3e, "???"  , ILLEGAL_OP     , 0 , 1},
    {0x3f, "swi"  , ADDR_INHERENT  , 19, 1},

    {0x40, "nega" , ADDR_INHERENT  , 2 , 1},
    {0x41, "???"  , ILLEGAL_OP     , 0 , 1},
    {0x42, "???"  , ILLEGAL_OP     , 0 , 1},
    {0x43, "coma" , ADDR_INHERENT  , 2 , 1},
    {0x44, "lsra" , ADDR_INHERENT  , 2 , 1},
    {0x45, "???"  , ILLEGAL_OP     , 0 , 1},
    {0x46, "rora" , ADDR_INHERENT  , 2 , 1},
    {0x47, "asra" , ADDR_INHERENT  , 2 , 1},
    {0x48, "asla" , ADDR_INHERENT  , 2 , 1},    // lsla
    {0x49, "rola" , ADDR_INHERENT  , 2 , 1},
    {0x4a, "deca" , ADDR_INHERENT  , 2 , 1},
    {0x4b, "???"  , ILLEGAL_OP     , 0 , 1},
    {0x4c, "inca" , ADDR_INHERENT  , 2 , 1},
    {0x4d, "tsta" , ADDR_INHERENT  , 2 , 1},
    {0x4e, "???"  , ILLEGAL_OP     , 0 , 1},
    {0x4f, "clra" , ADDR_INHERENT  , 2 , 1},

    {0x50, "negb" , ADDR_INHERENT  , 2 , 1},
    {0x51, "???"  , ILLEGAL_OP     , 0 , 1},
    {0x52, "???"  , ILLEGAL_OP     , 0 , 1},
    {0x53, "comb" , ADDR_INHERENT  , 2 , 1},
    {0x54, "lsrb" , ADDR_INHERENT  , 2 , 1},
    {0x55, "???"  , ILLEGAL_OP     , 0 , 1},
    {0x56, "rorb" , ADDR_INHERENT  , 2 , 1},
    {0x57, "asrb" , ADDR_INHERENT  , 2 , 1},
    {0x58, "aslb" , ADDR_INHERENT  , 2 , 1},    // lslb
    {0x59, "rolb" , ADDR_INHERENT  , 2 , 1},
    {0x5a, "decb" , ADDR_INHERENT  , 2 , 1},
    {0x5b, "???"  , ILLEGAL_OP     , 0 , 1},
    {0x5c, "incb" , ADDR_INHERENT  , 2 , 1},
    {0x5d, "tstb" , ADDR_INHERENT  , 2 , 1},
    {0x5e, "???"  , ILLEGAL_OP     , 0 , 1},
    {0x5f, "clrb" , ADDR_INHERENT  , 2 , 1},

    {0x60, "neg"  , ADDR_INDEXED   , 6 , 2},
    {0x61, "???"  , ILLEGAL_OP     , 0 , 1},
    {0x62, "???"  , ILLEGAL_OP     , 0 , 1},
    {0x63, "com"  , ADDR_INDEXED   , 6 , 2},
    {0x64, "lsr"  , ADDR_INDEXED   , 6 , 2},
    {0x65, "???"  , ILLEGAL_OP     , 0 , 1},
    {0x66, "ror"  , ADDR_INDEXED   , 6 , 2},
    {0x67, "asr"  , ADDR_INDEXED   , 6 , 2},
    {0x68, "asl"  , ADDR_INDEXED   , 6 , 2},    // lsl
    {0x69, "rol"  , ADDR_INDEXED   , 6 , 2},
    {0x6a, "dec"  , ADDR_INDEXED   , 6 , 2},
    {0x6b, "???"  , ILLEGAL_OP     , 0 , 1},
    {0x6c, "inc"  , ADDR_INDEXED   , 6 , 2},
    {0x6d, "tst"  , ADDR_INDEXED   , 6 , 2},
    {0x6e, "jmp"  , ADDR_INDEXED   , 3 , 2},
    {0x6f, "clr"  , ADDR_INDEXED   , 6 , 2},

    {0x70, "neg"  , ADDR_EXTENDED  , 7 , 3},
    {0x71, "???"  , ILLEGAL_OP     , 0 , 1},
    {0x72, "???"  , ILLEGAL_OP     , 0 , 1},
    {0x73, "com"  , ADDR_EXTENDED  , 7 , 3},
    {0x74, "lsr"  , ADDR_EXTENDED  , 7 , 3},
    {0x75, "???"  , ILLEGAL_OP     , 0 , 1},
    {0x76, "ror"  , ADDR_EXTENDED  , 7 , 3},
    {0x77, "asr"  , ADDR_EXTENDED  , 7 , 3},
    {0x78, "asl"  , ADDR_EXTENDED  , 7 , 3},    // lsl
    {0x79, "rol"  , ADDR_EXTENDED  , 7 , 3},
    {0x7a, "dec"  , ADDR_EXTENDED  , 7 , 3},
    {0x7b, "???"  , ILLEGAL_OP     , 0 , 1},
    {0x7c, "inc"  , ADDR_EXTENDED  , 7 , 3},
    {0x7d, "tst"  , ADDR_EXTENDED  , 7 , 3},
    {0x7e, "jmp"  , ADDR_EXTENDED  , 4 , 3},
    {0x7f, "clr"  , ADDR_EXTENDED  , 7 , 3},

    {0x80, "suba" , ADDR_IMMEDIATE , 2 , 2},
    {0x81, "cmpa" , ADDR_IMMEDIATE , 2 , 2},
    {0x82, "sbca" , ADDR_IMMEDIATE , 2 , 2},
    {0x83, "subd" , ADDR_LIMMEDIATE, 4 , 3},
    {0x84, "anda" , ADDR_IMMEDIATE , 2 , 2},
    {0x85, "bita" , ADDR_IMMEDIATE , 2 , 2},
    {0x86, "lda"  , ADDR_IMMEDIATE , 2 , 2},
    {0x87, "???"  , ILLEGAL_OP     , 0 , 1},
    {0x88, "eora" , ADDR_IMMEDIATE , 2 , 2},
    {0x89, "adca" , ADDR_IMMEDIATE , 2 , 2},
    {0x8a, "ora"  , ADDR_IMMEDIATE , 2 , 2},
    {0x8b, "adda" , ADDR_IMMEDIATE , 2 , 2},
    {0x8c, "cmpx" , ADDR_LIMMEDIATE, 4 , 3},
    {0x8d, "bsr"  , ADDR_RELATIVE  , 7 , 2},
    {0x8e, "ldx"  , ADDR_LIMMEDIATE, 3 , 3},
    {0x8f, "???"  , ILLEGAL_OP     , 0 , 1},

    {0x90, "suba" , ADDR_DIRECT    , 4 , 2},
    {0x91, "cmpa" , ADDR_DIRECT    , 4 , 2},
    {0x92, "sbca" , ADDR_DIRECT    , 4 , 2},
    {0x93, "subd" , ADDR_DIRECT    , 6 , 2},
    {0x94, "anda" , ADDR_DIRECT    , 4 , 2},
    {0x95, "bita" , ADDR_DIRECT    , 4 , 2},
    {0x96, "lda"  , ADDR_DIRECT    , 4 , 2},
    {0x97, "sta"  , ADDR_DIRECT    , 4 , 2},
    {0x98, "eora" , ADDR_DIRECT    , 4 , 2},
    {0x99, "adca" , ADDR_DIRECT    , 4 , 2},
    {0x9a, "ora"  , ADDR_DIRECT    , 4 , 2},
    {0x9b, "adda" , ADDR_DIRECT    , 4 , 2},
    {0x9c, "cmpx" , ADDR_DIRECT    , 6 , 2},
    {0x9d, "jsr"  , ADDR_DIRECT    , 7 , 2},
    {0x9e, "ldx"  , ADDR_DIRECT    , 5 , 2},
    {0x9f, "stx"  , ADDR_DIRECT    , 5 , 2},

    {0xa0, "suba" , ADDR_INDEXED   , 4 , 2},
    {0xa1, "cmpa" , ADDR_INDEXED   , 4 , 2},
    {0xa2, "sbca" , ADDR_INDEXED   , 4 , 2},
    {0xa3, "subd" , ADDR_INDEXED   , 6 , 2},
    {0xa4, "anda" , ADDR_INDEXED   , 4 , 2},
    {0xa5, "bita" , ADDR_INDEXED   , 4 , 2},
    {0xa6, "lda"  , ADDR_INDEXED   , 4 , 2},
    {0xa7, "sta"  , ADDR_INDEXED   , 4 , 2},
    {0xa8, "eora" , ADDR_INDEXED   , 4 , 2},
    {0xa9, "adca" , ADDR_INDEXED   , 4 , 2},
    {0xaa, "ora"  , ADDR_INDEXED   , 4 , 2},
    {0xab, "adda" , ADDR_INDEXED   , 4 , 2},
    {0xac, "cmpx" , ADDR_INDEXED   , 6 , 2},
    {0xad, "jsr"  , ADDR_INDEXED   , 7 , 2},
    {0xae, "ldx"  , ADDR_INDEXED   , 5 , 2},
    {0xaf, "stx"  , ADDR_INDEXED   , 5 , 2},

    {0xb0, "suba" , ADDR_EXTENDED  , 5 , 3},
    {0xb1, "cmpa" , ADDR_EXTENDED  , 5 , 3},
    {0xb2, "sbca" , ADDR_EXTENDED  , 5 , 3},
    {0xb3, "subd" , ADDR_EXTENDED  , 7 , 3},
    {0xb4, "anda" , ADDR_EXTENDED  , 5 , 3},
    {0xb5, "bita" , ADDR_EXTENDED  , 5 , 3},
    {0xb6, "lda"  , ADDR_EXTENDED  , 5 , 3},
    {0xb7, "sta"  , ADDR_EXTENDED  , 5 , 3},
    {0xb8, "eora" , ADDR_EXTENDED  , 5 , 3},
    {0xb9, "adca" , ADDR_EXTENDED  , 5 , 3},
    {0xba, "ora"  , ADDR_EXTENDED  , 5 , 3},
    {0xbb, "adda" , ADDR_EXTENDED  , 5 , 3},
    {0xbc, "cmpx" , ADDR_EXTENDED  , 7 , 3},
    {0xbd, "jsr"  , ADDR_EXTENDED  , 8 , 3},
    {0xbe, "ldx"  , ADDR_EXTENDED  , 6 , 3},
    {0xbf, "stx"  , ADDR_EXTENDED  , 6 , 3},

    {0xc0, "subb" , ADDR_IMMEDIATE , 2 , 2},
    {0xc1, "cmpb" , ADDR_IMMEDIATE , 2 , 2},
    {0xc2, "sbcb" , ADDR_IMMEDIATE , 2 , 2},
    {0xc3, "addd" , ADDR_LIMMEDIATE, 4 , 3},
    {0xc4, "andb" , ADDR_IMMEDIATE , 2 , 2},
    {0xc5, "bitb" , ADDR_IMMEDIATE , 2 , 2},
    {0xc6, "ldb"  , ADDR_IMMEDIATE , 2 , 2},
    {0xc7, "???"  , ILLEGAL_OP     , 0 , 1},
    {0xc8, "eorb" , ADDR_IMMEDIATE , 2 , 2},
    {0xc9, "adcb" , ADDR_IMMEDIATE , 2 , 2},
    {0xca, "orb"  , ADDR_IMMEDIATE , 2 , 2},
    {0xcb, "addb" , ADDR_IMMEDIATE , 2 , 2},
    {0xcc, "ldd"  , ADDR_LIMMEDIATE, 3 , 3},
    {0xcd, "???"  , ILLEGAL_OP     , 0 , 1},
    {0xce, "ldu"  , ADDR_LIMMEDIATE, 3 , 3},
    {0xcf, "???"  , ILLEGAL_OP     , 0 , 1},

    {0xd0, "subb" , ADDR_DIRECT    , 4 , 2},
    {0xd1, "cmpb" , ADDR_DIRECT    , 4 , 2},
    {0xd2, "sbcb" , ADDR_DIRECT    , 4 , 2},
    {0xd3, "addd" , ADDR_DIRECT    , 6 , 2},
    {0xd4, "andb" , ADDR_DIRECT    , 4 , 2},
    {0xd5, "bitb" , ADDR_DIRECT    , 4 , 2},
    {0xd6, "ldb"  , ADDR_DIRECT    , 4 , 2},
    {0xd7, "stb"  , ADDR_DIRECT    , 4 , 2},
    {0xd8, "eorb" , ADDR_DIRECT    , 4 , 2},
    {0xd9, "adcb" , ADDR_DIRECT    , 4 , 2},
    {0xda, "orb"  , ADDR_DIRECT    , 4 , 2},
    {0xdb, "addb" , ADDR_DIRECT    , 4 , 2},
    {0xdc, "ldd"  , ADDR_DIRECT    , 5 , 2},
    {0xdd, "std"  , ADDR_DIRECT    , 5 , 2},
    {0xde, "ldu"  , ADDR_DIRECT    , 5 , 2},
    {0xdf, "stu"  , ADDR_DIRECT    , 5 , 2},

    {0xe0, "subb" , ADDR_INDEXED   , 4 , 2},
    {0xe1, "cmpb" , ADDR_INDEXED   , 4 , 2},
    {0xe2, "sbcb" , ADDR_INDEXED   , 4 , 2},
    {0xe3, "addd" , ADDR_INDEXED   , 6 , 2},
    {0xe4, "andb" , ADDR_INDEXED   , 4 , 2},
    {0xe5, "bitb" , ADDR_INDEXED   , 4 , 2},
    {0xe6, "ldb"  , ADDR_INDEXED   , 4 , 2},
    {0xe7, "stb"  , ADDR_INDEXED   , 4 , 2},
    {0xe8, "eorb" , ADDR_INDEXED   , 4 , 2},
    {0xe9, "adcb" , ADDR_INDEXED   , 4 , 2},
    {0xea, "orb"  , ADDR_INDEXED   , 4 , 2},
    {0xeb, "addb" , ADDR_INDEXED   , 4 , 2},
    {0xec, "ldd"  , ADDR_INDEXED   , 5 , 2},
    {0xed, "std"  , ADDR_INDEXED   , 5 , 2},
    {0xee, "ldu"  , ADDR_INDEXED   , 5 , 2},
    {0xef, "stu"  , ADDR_INDEXED   , 5 , 2},

    {0xf0, "subb" , ADDR_EXTENDED  , 5 , 3},
    {0xf1, "cmpb" , ADDR_EXTENDED  , 5 , 3},
    {0xf2, "sbcb" , ADDR_EXTENDED  , 5 , 3},
    {0xf3, "addd" , ADDR_EXTENDED  , 7 , 3},
    {0xf4, "andb" , ADDR_EXTENDED  , 5 , 3},
    {0xf5, "bitb" , ADDR_EXTENDED  , 5 , 3},
    {0xf6, "ldb"  , ADDR_EXTENDED  , 5 , 3},
    {0xf7, "stb"  , ADDR_EXTENDED  , 5 , 3},
    {0xf8, "eorb" , ADDR_EXTENDED  , 5 , 3},
    {0xf9, "adcb" , ADDR_EXTENDED  , 5 , 3},
    {0xfa, "orb"  , ADDR_EXTENDED  , 5 , 3},
    {0xfb, "addb" , ADDR_EXTENDED  , 5 , 3},
    {0xfc, "ldd"  , ADDR_EXTENDED  , 6 , 3},
    {0xfd, "std"  , ADDR_EXTENDED  , 6 , 3},
    {0xfe, "ldu"  , ADDR_EXTENDED  , 6 , 3},
    {0xff, "stu"  , ADDR_EXTENDED  , 6 , 3},

    /* Double byte 0x10 op-codes
     * Index 256 to 293
     */
    {0x21, "lbrn" , ADDR_LRELATIVE , 5 , 4},
    {0x22, "lbhi" , ADDR_LRELATIVE , 5 , 4},
    {0x23, "lbls" , ADDR_LRELATIVE , 5 , 4},
    {0x24, "lbcc" , ADDR_LRELATIVE , 5 , 4},    // lbhs
    {0x25, "lbcs" , ADDR_LRELATIVE , 5 , 4},    // lblo
    {0x26, "lbne" , ADDR_LRELATIVE , 5 , 4},
    {0x27, "lbeq" , ADDR_LRELATIVE , 5 , 4},
    {0x28, "lbvc" , ADDR_LRELATIVE , 5 , 4},
    {0x29, "lbvs" , ADDR_LRELATIVE , 5 , 4},
    {0x2a, "lbpl" , ADDR_LRELATIVE , 5 , 4},
    {0x2b, "lbmi" , ADDR_LRELATIVE , 5 , 4},
    {0x2c, "lbge" , ADDR_LRELATIVE , 5 , 4},
    {0x2d, "lblt" , ADDR_LRELATIVE , 5 , 4},
    {0x2e, "lbgt" , ADDR_LRELATIVE , 5 , 4},
    {0x2f, "lble" , ADDR_LRELATIVE , 5 , 4},
    {0x3f, "swi2" , ADDR_INHERENT  , 20, 2},
    {0x83, "cmpd" , ADDR_LIMMEDIATE, 5 , 4},
    {0x8c, "cmpy" , ADDR_LIMMEDIATE, 5 , 4},
    {0x8e, "ldy"  , ADDR_LIMMEDIATE, 4 , 4},
    {0x93, "cmpd" , ADDR_DIRECT    , 7 , 3},
    {0x9c, "cmpy" , ADDR_DIRECT    , 7 , 3},
    {0x9e, "ldy"  , ADDR_DIRECT    , 6 , 3},
    {0x9f, "sty"  , ADDR_DIRECT    , 6 , 3},
    {0xa3, "cmpd" , ADDR_INDEXED   , 7 , 3},
    {0xac, "cmpy" , ADDR_INDEXED   , 7 , 3},
    {0xae, "ldy"  , ADDR_INDEXED   , 6 , 3},
    {0xaf, "sty"  , ADDR_INDEXED   , 6 , 3},
    {0xb3, "cmpd" , ADDR_EXTENDED  , 8 , 4},
    {0xbc, "cmpy" , ADDR_EXTENDED  , 8 , 4},
    {0xbe, "ldy"  , ADDR_EXTENDED  , 7 , 4},
    {0xbf, "sty"  , ADDR_EXTENDED  , 7 , 4},
    {0xce, "lds"  , ADDR_LIMMEDIATE, 4 , 4},
    {0xde, "lds"  , ADDR_DIRECT    , 6 , 3},
    {0xdf, "sts"  , ADDR_DIRECT    , 6 , 3},
    {0xee, "lds"  , ADDR_INDEXED   , 6 , 3},
    {0xef, "sts"  , ADDR_INDEXED   , 6 , 3},
    {0xfe, "lds"  , ADDR_EXTENDED  , 7 , 4},
    {0xff, "sts"  , ADDR_EXTENDED  , 7 , 4},

    /* Double byte 0x11 op-codes
     * Index 294 to 302
     */
    {0xef, "swi3" , ADDR_INHERENT  , 20, 2},
    {0x83, "cmpu" , ADDR_LIMMEDIATE, 5 , 4},
    {0x8c, "cmps" , ADDR_LIMMEDIATE, 5 , 4},
    {0x93, "cmpu" , ADDR_DIRECT    , 7 , 3},
    {0x9c, "cmps" , ADDR_DIRECT    , 7 , 3},
    {0xa3, "cmpu" , ADDR_INDEXED   , 7 , 3},
    {0xac, "cmps" , ADDR_INDEXED   , 7 , 3},
    {0xb3, "cmpu" , ADDR_EXTENDED  , 8 , 4},
    {0xbc, "cmps" , ADDR_EXTENDED  , 8 , 4},
};

#endif  /* __MC6809E_H__ */


#ifndef LEX_H
#define LEX_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>

typedef struct
{
  uint16_t type;  // instr    | reg     | mem      | const
  uint16_t value; // instr_id | reg_num | mem_addr | value
  uint16_t flags; // used for condition flags of jmp
}
Token;

typedef uint32_t MicroInstruction;

// token types

#define TT_INSTR     0  // mov
#define TT_REG       1  // R0
#define TT_REG_MEM   2  // M[R0]
#define TT_MEM       3  // M[2]
#define TT_CONST     4  // 0|A|B|C  (zero or instruction operands)
#define TT_EOF       5  // EOF
#define TT_DIR       6  // .org
#define TT_ADDR      7  // xfF
#define TT_LABEL_DEF 8  // label:
#define TT_LABEL     9  // label

static const Token TOKEN_EOF = { TT_EOF, 0 };

#define BUF_SIZE 50

#define ROM_SIZE 256

#define MINSTR_BYTE_SIZE 3


typedef struct Label
{
  char label[BUF_SIZE];
  uint8_t pos;
  
  struct Label* next; // next label
}
Label;

typedef struct LabelFixup
{
  char label[BUF_SIZE];
  uint8_t instr_offset;     // the offset of the MODE=1 instruction waiting on
                            // the address for its NXT_ADDR field
  struct LabelFixup* next;
}
LabelFixup;

typedef struct LexState
{
  FILE* in;
  int line;
  int column;
  int buf_len;
  uint8_t instr_pos;
  
  char buf[BUF_SIZE];

  /** the memory layout of instructions */
  MicroInstruction instructions[ROM_SIZE];
  
  Label* labels;
  LabelFixup* fixups;
}
*LexState;

void add_label( char* label, uint8_t pos, LexState state );

/**
* Returns either the address associated with the given label,
* or -1 if the label was not found
*/
int find_label( char* label, LexState state );

/**
* Add the given instruction to the list of label fix ups
*/
void fixup_label( char* label, uint8_t instr_offset, LexState state );



static const int MNEMONIC_COUNT = 0xe;

static const char* MNEMONICS[] = {
  "mov",
  "add",
  "sub",
  "mul",
  "rsh",
  "not",
  "and",
  "div",
  "or",
  "nadd",
  "lsh",
  "sar",
  "jmp",
  "nop",
  ".org"
};

// Mnemonics

#define MN_MOV    0x0
#define MN_ADD    0x1
#define MN_SUB    0x2
#define MN_MUL    0x3
#define MN_RSH    0x4
#define MN_NOT    0x5
#define MN_AND    0x6
#define MN_DIV    0x7
#define MN_OR     0x8
#define MN_NADD   0x9
#define MN_LSH    0xa
#define MN_SAR    0xb
#define MN_JMP    0xc
#define MN_NOP    0xd

// Directives
#define DR_ORG    0xe


#define CONST_0  0x0
#define CONST_A  0x1
#define CONST_B  0x2
#define CONST_C  0x3
#define CONST_1  0x4

#define COND_P 0x4
#define COND_Z 0x2
#define COND_N 0x1


/**

Instruction:
16-bit

+----+----+----+----+
| OP |  A |  B |  C |
+----+----+----+----+

OP:    OPCODE
A,B,C: 16-bit memory addresses

form: INSTR A B C




========================

Control Word: 16-bit
+---+-+---+-+---+---+-+
| A |M| B |M| F | D |R|
| A |B| A |F| S | A |W|
+---+-+---+-+---+---+-+

AA: register output A Address
MB: 0: use B register output
    1: use constant in
BA: register output B Address
MF: 0: send output of function unit to register
    1: send response from memory unit to register
FS: Function Select
DA: address of register to write
RW: 'write register' flag
    0: register file changes disabled
    1: register at DA will be modified
MW: 'write memory' flag
    0: memory at AddrOut will NOT be modified
    1: memory at AddrOut will be set to value of DataOut

    
    
Microinstruction: 18-bit

Microoperation (high bit 0)
+-+-+----------------+
|0|M|     Control    |
| |W|      Word      |
+-+-+----------------+

Microsequencing (high bit 1)
+-+--+---+--------+----+
|1|00|CND|NXT_ADDR|0000|
+-+--+---+--------+----+

CND: condition flags (PZN)
NXT_ADDR: address to jump based on condition flags
    
instruction forms:
mov reg mem
mov mem reg
mov reg const

x04 add  dst reg reg
x05 sub  dst reg reg
x06 mul  dst reg reg
x07 div  dst reg reg
x08 not  dst reg
x09 and  dst reg reg
x10 or   dst reg reg
x11 nadd dst reg
x12 rsh  dst src
x13 lsh  dst src
x14 sar  dst src

reg = { R0, R1, R2, R3, R4, R5, R6, R7 }
mem = { [R0], [R1], [R2], [R3] }
const = { 0, 1, b0101, x4e }





*/

// micro instruction masks
#define M_MODE 0x20000

// micro operation fields
#define M_MW   0x10000
#define M_AA   0x0E000
#define M_MB   0x01000
#define M_BA   0x00E00
#define M_MF   0x00100
#define M_FS   0x000F0
#define M_DA   0x0000E
#define M_RW   0x00001

// micro sequencing fields
#define M_COND      0x07000
#define M_NEXT_ADDR 0x00FF0

// Control Functions
#define F_0        0x0
#define F_1        0x1
#define F_A        0x2
#define F_B        0x3
#define F_ADD      0x4
#define F_SUB      0x5
#define F_MUL      0x6
#define F_DIV      0x7
#define F_NOT      0x8
#define F_AND      0x9
#define F_OR       0xa
#define F_NADD     0xb
#define F_RSH      0xc
#define F_LSH      0xd
#define F_SAR      0xe
#define F_MOV      0xf

/*********
 Tokenize
**********/

void error( char* msg, LexState state );

int read( LexState state );

void unread( int c, LexState state );

int peek( LexState state );

void expect( int expected, LexState state );

void skip_ws( LexState state );

uint16_t read_hex( LexState state );


Token read_token( LexState state );

#endif
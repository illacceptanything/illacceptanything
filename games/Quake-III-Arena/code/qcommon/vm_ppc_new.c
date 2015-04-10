/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Foobar; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
// vm_ppc.c
// ppc dynamic compiler

#include "vm_local.h"

#pragma opt_pointer_analysis off

#define DEBUG_VM 0

#if DEBUG_VM
static char	*opnames[256] = {
	"OP_UNDEF", 

	"OP_IGNORE", 

	"OP_BREAK",

	"OP_ENTER",
	"OP_LEAVE",
	"OP_CALL",
	"OP_PUSH",
	"OP_POP",

	"OP_CONST",

	"OP_LOCAL",

	"OP_JUMP",

	//-------------------

	"OP_EQ",
	"OP_NE",

	"OP_LTI",
	"OP_LEI",
	"OP_GTI",
	"OP_GEI",

	"OP_LTU",
	"OP_LEU",
	"OP_GTU",
	"OP_GEU",

	"OP_EQF",
	"OP_NEF",

	"OP_LTF",
	"OP_LEF",
	"OP_GTF",
	"OP_GEF",

	//-------------------

	"OP_LOAD1",
	"OP_LOAD2",
	"OP_LOAD4",
	"OP_STORE1",
	"OP_STORE2",
	"OP_STORE4",
	"OP_ARG",

	"OP_BLOCK_COPY",

	//-------------------

	"OP_SEX8",
	"OP_SEX16",

	"OP_NEGI",
	"OP_ADD",
	"OP_SUB",
	"OP_DIVI",
	"OP_DIVU",
	"OP_MODI",
	"OP_MODU",
	"OP_MULI",
	"OP_MULU",

	"OP_BAND",
	"OP_BOR",
	"OP_BXOR",
	"OP_BCOM",

	"OP_LSH",
	"OP_RSHI",
	"OP_RSHU",

	"OP_NEGF",
	"OP_ADDF",
	"OP_SUBF",
	"OP_DIVF",
	"OP_MULF",

	"OP_CVIF",
	"OP_CVFI"
};
#endif

typedef enum {
    R_REAL_STACK = 1,
	// registers 3-11 are the parameter passing registers
	
	// state
	R_STACK = 3,			// local
	R_OPSTACK,				// global

	// constants	
	R_MEMBASE,			// global
	R_MEMMASK,
	R_ASMCALL,			// global
    R_INSTRUCTIONS,		// global
    R_NUM_INSTRUCTIONS,	// global
    R_CVM,				// currentVM
    
	// temps
	R_TOP = 11,
	R_SECOND = 12,
	R_EA = 2				// effective address calculation
	 
} regNums_t;

#define	RG_REAL_STACK		r1
#define	RG_STACK			r3
#define	RG_OPSTACK			r4
#define	RG_MEMBASE			r5
#define	RG_MEMMASK			r6
#define	RG_ASMCALL			r7
#define	RG_INSTRUCTIONS		r8
#define	RG_NUM_INSTRUCTIONS	r9
#define	RG_CVM				r10
#define	RG_TOP				r12
#define	RG_SECOND			r13
#define	RG_EA 				r14

// The deepest value I saw in the Quake3 games was 9.
#define OP_STACK_MAX_DEPTH	12

// These are all volatile and thus must be saved
// upon entry to the VM code.
static int opStackIntRegisters[OP_STACK_MAX_DEPTH] =
{
	16, 17, 18, 19,
	20, 21, 22, 23,
	24, 25, 26, 27
};

static unsigned int *opStackLoadInstructionAddr[OP_STACK_MAX_DEPTH];

// We use different registers for the floating point
// operand stack (these are volatile in the PPC ABI)
static int opStackFloatRegisters[OP_STACK_MAX_DEPTH] =
{
	0, 1, 2, 3,
	4, 5, 6, 7,
	8, 9, 10, 11
};

static int opStackRegType[OP_STACK_MAX_DEPTH] =
{
	0, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0
};

// this doesn't have the low order bits set for instructions i'm not using...
typedef enum {
	PPC_TDI		= 0x08000000,
	PPC_TWI		= 0x0c000000,
	PPC_MULLI	= 0x1c000000,
	PPC_SUBFIC	= 0x20000000,
	PPC_CMPI	= 0x28000000,
	PPC_CMPLI	= 0x2c000000,
	PPC_ADDIC	= 0x30000000,
	PPC_ADDIC_	= 0x34000000,
	PPC_ADDI	= 0x38000000,
	PPC_ADDIS	= 0x3c000000,
	PPC_BC		= 0x40000000,
	PPC_SC		= 0x44000000,
	PPC_B		= 0x48000000,

	PPC_MCRF	= 0x4c000000,
	PPC_BCLR	= 0x4c000020,
	PPC_RFID	= 0x4c000000,
	PPC_CRNOR	= 0x4c000000,
	PPC_RFI		= 0x4c000000,
	PPC_CRANDC	= 0x4c000000,
	PPC_ISYNC	= 0x4c000000,
	PPC_CRXOR	= 0x4c000000,
	PPC_CRNAND	= 0x4c000000,
	PPC_CREQV	= 0x4c000000,
	PPC_CRORC	= 0x4c000000,
	PPC_CROR	= 0x4c000000,
//------------
	PPC_BCCTR	= 0x4c000420,
	PPC_RLWIMI	= 0x50000000,
	PPC_RLWINM	= 0x54000000,
	PPC_RLWNM	= 0x5c000000,
	PPC_ORI		= 0x60000000,
	PPC_ORIS	= 0x64000000,
	PPC_XORI	= 0x68000000,
	PPC_XORIS	= 0x6c000000,
	PPC_ANDI_	= 0x70000000,
	PPC_ANDIS_	= 0x74000000,
	PPC_RLDICL	= 0x78000000,
	PPC_RLDICR	= 0x78000000,
	PPC_RLDIC	= 0x78000000,
	PPC_RLDIMI	= 0x78000000,
	PPC_RLDCL	= 0x78000000,
	PPC_RLDCR	= 0x78000000,
	PPC_CMP		= 0x7c000000,
	PPC_TW		= 0x7c000000,
	PPC_SUBFC	= 0x7c000010,
	PPC_MULHDU	= 0x7c000000,
	PPC_ADDC	= 0x7c000014,
	PPC_MULHWU	= 0x7c000000,
	PPC_MFCR	= 0x7c000000,
	PPC_LWAR	= 0x7c000000,
	PPC_LDX		= 0x7c000000,
	PPC_LWZX	= 0x7c00002e,
	PPC_SLW		= 0x7c000030,
	PPC_CNTLZW	= 0x7c000000,
	PPC_SLD		= 0x7c000000,
	PPC_AND		= 0x7c000038,
	PPC_CMPL	= 0x7c000040,
	PPC_SUBF	= 0x7c000050,
	PPC_LDUX	= 0x7c000000,
//------------
	PPC_DCBST	= 0x7c000000,
	PPC_LWZUX	= 0x7c00006c,
	PPC_CNTLZD	= 0x7c000000,
	PPC_ANDC	= 0x7c000000,
	PPC_TD		= 0x7c000000,
	PPC_MULHD	= 0x7c000000,
	PPC_MULHW	= 0x7c000000,
	PPC_MTSRD	= 0x7c000000,
	PPC_MFMSR	= 0x7c000000,
	PPC_LDARX	= 0x7c000000,
	PPC_DCBF	= 0x7c000000,
	PPC_LBZX	= 0x7c0000ae,
	PPC_NEG		= 0x7c000000,
	PPC_MTSRDIN	= 0x7c000000,
	PPC_LBZUX	= 0x7c000000,
	PPC_NOR		= 0x7c0000f8,
	PPC_SUBFE	= 0x7c000000,
	PPC_ADDE	= 0x7c000000,
	PPC_MTCRF	= 0x7c000000,
	PPC_MTMSR	= 0x7c000000,
	PPC_STDX	= 0x7c000000,
	PPC_STWCX_	= 0x7c000000,
	PPC_STWX	= 0x7c00012e,
	PPC_MTMSRD	= 0x7c000000,
	PPC_STDUX	= 0x7c000000,
	PPC_STWUX	= 0x7c00016e,
	PPC_SUBFZE	= 0x7c000000,
	PPC_ADDZE	= 0x7c000000,
	PPC_MTSR	= 0x7c000000,
	PPC_STDCX_	= 0x7c000000,
	PPC_STBX	= 0x7c0001ae,
	PPC_SUBFME	= 0x7c000000,
	PPC_MULLD	= 0x7c000000,
//------------
	PPC_ADDME	= 0x7c000000,
	PPC_MULLW	= 0x7c0001d6,
	PPC_MTSRIN	= 0x7c000000,
	PPC_DCBTST	= 0x7c000000,
	PPC_STBUX	= 0x7c000000,
	PPC_ADD		= 0x7c000214,
	PPC_DCBT	= 0x7c000000,
	PPC_LHZX	= 0x7c00022e,
	PPC_EQV		= 0x7c000000,
	PPC_TLBIE	= 0x7c000000,
	PPC_ECIWX	= 0x7c000000,
	PPC_LHZUX	= 0x7c000000,
	PPC_XOR		= 0x7c000278,
	PPC_MFSPR	= 0x7c0002a6,
	PPC_LWAX	= 0x7c000000,
	PPC_LHAX	= 0x7c000000,
	PPC_TLBIA	= 0x7c000000,
	PPC_MFTB	= 0x7c000000,
	PPC_LWAUX	= 0x7c000000,
	PPC_LHAUX	= 0x7c000000,
	PPC_STHX	= 0x7c00032e,
	PPC_ORC		= 0x7c000338,
	PPC_SRADI	= 0x7c000000,
	PPC_SLBIE	= 0x7c000000,
	PPC_ECOWX	= 0x7c000000,
	PPC_STHUX	= 0x7c000000,
	PPC_OR		= 0x7c000378,
	PPC_DIVDU	= 0x7c000000,
	PPC_DIVWU	= 0x7c000396,
	PPC_MTSPR	= 0x7c0003a6,
	PPC_DCBI	= 0x7c000000,
	PPC_NAND	= 0x7c000000,
	PPC_DIVD	= 0x7c000000,
//------------
	PPC_DIVW	= 0x7c0003d6,
	PPC_SLBIA	= 0x7c000000,
	PPC_MCRXR	= 0x7c000000,
	PPC_LSWX	= 0x7c000000,
	PPC_LWBRX	= 0x7c000000,
	PPC_LFSX	= 0x7c00042e,
	PPC_SRW		= 0x7c000430,
	PPC_SRD		= 0x7c000000,
	PPC_TLBSYNC	= 0x7c000000,
	PPC_LFSUX	= 0x7c000000,
	PPC_MFSR	= 0x7c000000,
	PPC_LSWI	= 0x7c000000,
	PPC_SYNC	= 0x7c000000,
	PPC_LFDX	= 0x7c000000,
	PPC_LFDUX	= 0x7c000000,
	PPC_MFSRIN	= 0x7c000000,
	PPC_STSWX	= 0x7c000000,
	PPC_STWBRX	= 0x7c000000,
	PPC_STFSX	= 0x7c00052e,
	PPC_STFSUX	= 0x7c000000,
	PPC_STSWI	= 0x7c000000,
	PPC_STFDX	= 0x7c000000,
	PPC_DCBA	= 0x7c000000,
	PPC_STFDUX	= 0x7c000000,
	PPC_LHBRX	= 0x7c000000,
	PPC_SRAW	= 0x7c000630,
	PPC_SRAD	= 0x7c000000,
	PPC_SRAWI	= 0x7c000000,
	PPC_EIEIO	= 0x7c000000,
	PPC_STHBRX	= 0x7c000000,
	PPC_EXTSH	= 0x7c000734,
	PPC_EXTSB	= 0x7c000774,
	PPC_ICBI	= 0x7c000000,
//------------
	PPC_STFIWX	= 0x7c0007ae,
	PPC_EXTSW	= 0x7c000000,
	PPC_DCBZ	= 0x7c000000,
	PPC_LWZ		= 0x80000000,
	PPC_LWZU	= 0x84000000,
	PPC_LBZ		= 0x88000000,
	PPC_LBZU	= 0x8c000000,
	PPC_STW		= 0x90000000,
	PPC_STWU	= 0x94000000,
	PPC_STB		= 0x98000000,
	PPC_STBU	= 0x9c000000,
	PPC_LHZ		= 0xa0000000,
	PPC_LHZU	= 0xa4000000,
	PPC_LHA		= 0xa8000000,
	PPC_LHAU	= 0xac000000,
	PPC_STH		= 0xb0000000,
	PPC_STHU	= 0xb4000000,
	PPC_LMW		= 0xb8000000,
	PPC_STMW	= 0xbc000000,
	PPC_LFS		= 0xc0000000,
	PPC_LFSU	= 0xc4000000,
	PPC_LFD		= 0xc8000000,
	PPC_LFDU	= 0xcc000000,
	PPC_STFS	= 0xd0000000,
	PPC_STFSU	= 0xd4000000,
	PPC_STFD	= 0xd8000000,
	PPC_STFDU	= 0xdc000000,
	PPC_LD		= 0xe8000000,
	PPC_LDU		= 0xe8000001,
	PPC_LWA		= 0xe8000002,
	PPC_FDIVS	= 0xec000024,
	PPC_FSUBS	= 0xec000028,
	PPC_FADDS	= 0xec00002a,
//------------
	PPC_FSQRTS	= 0xec000000,
	PPC_FRES	= 0xec000000,
	PPC_FMULS	= 0xec000032,
	PPC_FMSUBS	= 0xec000000,
	PPC_FMADDS	= 0xec000000,
	PPC_FNMSUBS	= 0xec000000,
	PPC_FNMADDS	= 0xec000000,
	PPC_STD		= 0xf8000000,
	PPC_STDU	= 0xf8000001,
	PPC_FCMPU	= 0xfc000000,
	PPC_FRSP	= 0xfc000018,
	PPC_FCTIW	= 0xfc000000,
	PPC_FCTIWZ	= 0xfc00001e,
	PPC_FDIV	= 0xfc000000,
	PPC_FSUB	= 0xfc000028,
	PPC_FADD	= 0xfc000000,
	PPC_FSQRT	= 0xfc000000,
	PPC_FSEL	= 0xfc000000,
	PPC_FMUL	= 0xfc000000,
	PPC_FRSQRTE	= 0xfc000000,
	PPC_FMSUB	= 0xfc000000,
	PPC_FMADD	= 0xfc000000,
	PPC_FNMSUB	= 0xfc000000,
	PPC_FNMADD	= 0xfc000000,
	PPC_FCMPO	= 0xfc000000,
	PPC_MTFSB1	= 0xfc000000,
	PPC_FNEG	= 0xfc000050,
	PPC_MCRFS	= 0xfc000000,
	PPC_MTFSB0	= 0xfc000000,
	PPC_FMR		= 0xfc000000,
	PPC_MTFSFI	= 0xfc000000,
	PPC_FNABS	= 0xfc000000,
	PPC_FABS	= 0xfc000000,
//------------
	PPC_MFFS	= 0xfc000000,
	PPC_MTFSF	= 0xfc000000,
	PPC_FCTID	= 0xfc000000,
	PPC_FCTIDZ	= 0xfc000000,
	PPC_FCFID	= 0xfc000000
	
} ppcOpcodes_t;


// the newly generated code
static	unsigned	*buf;
static	int		compiledOfs;	// in dwords
static int pass;

// fromt the original bytecode
static	byte	*code;
static	int		pc;

void AsmCall( void );

double	itofConvert[2];

static int	Constant4( void ) {
	int		v;

	v = code[pc] | (code[pc+1]<<8) | (code[pc+2]<<16) | (code[pc+3]<<24);
	pc += 4;
	return v;
}

static int	Constant1( void ) {
	int		v;

	v = code[pc];
	pc += 1;
	return v;
}

static void Emit4( char *opname, int i ) {
	#if DEBUG_VM
	if(pass == 1)
		printf("\t\t\t%p %s\t%08lx\n",&buf[compiledOfs],opname,i&0x3ffffff);
	#endif
	buf[ compiledOfs ] = i;
	compiledOfs++;
}

static void Inst( char *opname, int opcode, int destReg, int aReg, int bReg ) {
    unsigned		r;

	#if DEBUG_VM
	if(pass == 1)
		printf("\t\t\t%p %s\tr%d,r%d,r%d\n",&buf[compiledOfs],opname,destReg,aReg,bReg);
	#endif
    r = opcode | ( destReg << 21 ) | ( aReg << 16 ) | ( bReg << 11 ) ;
    buf[ compiledOfs ] = r;
    compiledOfs++;
}

static void Inst4( char *opname, int opcode, int destReg, int aReg, int bReg, int cReg ) {
    unsigned		r;

	#if DEBUG_VM
	if(pass == 1)
		printf("\t\t\t%p %s\tr%d,r%d,r%d,r%d\n",&buf[compiledOfs],opname,destReg,aReg,bReg,cReg);
	#endif
    r = opcode | ( destReg << 21 ) | ( aReg << 16 ) | ( bReg << 11 ) | ( cReg << 6 );
    buf[ compiledOfs ] = r;
    compiledOfs++;
}

static void InstImm( char *opname, int opcode, int destReg, int aReg, int immediate ) {
	unsigned		r;
	
	if ( immediate > 32767 || immediate < -32768 ) {
	    Com_Error( ERR_FATAL, "VM_Compile: immediate value %i out of range, opcode %x,%d,%d", immediate, opcode, destReg, aReg );
	}
	#if DEBUG_VM
	if(pass == 1)
		printf("\t\t\t%p %s\tr%d,r%d,0x%x\n",&buf[compiledOfs],opname,destReg,aReg,immediate);
	#endif
	r = opcode | ( destReg << 21 ) | ( aReg << 16 ) | ( immediate & 0xffff );
	buf[ compiledOfs ] = r;
	compiledOfs++;
}

static void InstImmU( char *opname, int opcode, int destReg, int aReg, int immediate ) {
	unsigned		r;
	
	if ( immediate > 0xffff || immediate < 0 ) {
		Com_Error( ERR_FATAL, "VM_Compile: immediate value %i out of range", immediate );
	}
	#if DEBUG_VM
	if(pass == 1)
		printf("\t\t\t%p %s\tr%d,r%d,0x%x\n",&buf[compiledOfs],opname,destReg,aReg,immediate);
	#endif
    r = opcode | ( destReg << 21 ) | ( aReg << 16 ) | ( immediate & 0xffff );
	buf[ compiledOfs ] = r;
	compiledOfs++;
}

static int pop0, pop1, oc0, oc1;
static vm_t *tvm;
static int instruction;
static byte *jused;

static void ltop() {
//    if (rtopped == qfalse) {
//	InstImm( PPC_LWZ, R_TOP, R_OPSTACK, 0 );		// get value from opstack
//    }
}

static void ltopandsecond() {
#if 0
    if (pass>=0 && buf[compiledOfs-1] == (PPC_STWU |  R_TOP<<21 | R_OPSTACK<<16 | 4 ) && jused[instruction]==0 ) {
	compiledOfs--;
	if (!pass) {
	    tvm->instructionPointers[instruction] = compiledOfs * 4;
	}
	InstImm( PPC_LWZ, R_SECOND, R_OPSTACK, 0 );	// get value from opstack
	InstImm( PPC_ADDI, R_OPSTACK, R_OPSTACK, -4 );
    } else if (pass>=0 && buf[compiledOfs-1] == (PPC_STW |  R_TOP<<21 | R_OPSTACK<<16 | 0 )  && jused[instruction]==0 ) {
	compiledOfs--;
	if (!pass) {
	    tvm->instructionPointers[instruction] = compiledOfs * 4;
	}
	InstImm( PPC_LWZ, R_SECOND, R_OPSTACK, -4 );	// get value from opstack
	InstImm( PPC_ADDI, R_OPSTACK, R_OPSTACK, -8 );
    } else {
	ltop();		// get value from opstack
	InstImm( PPC_LWZ, R_SECOND, R_OPSTACK, -4 );	// get value from opstack
	InstImm( PPC_ADDI, R_OPSTACK, R_OPSTACK, -8 );
    }
    rtopped = qfalse;
#endif
}

static void spillOpStack(int depth)
{
	// Store out each register on the operand stack to it's correct location.
	int i;
	
	for(i = 0; i < depth; i++)
	{
		assert(opStackRegType[i]);
		assert(opStackRegType[i] == 1);
		switch(opStackRegType[i])
		{
			case 1:	// Integer register
				InstImm( "stw", PPC_STW, opStackIntRegisters[i], R_OPSTACK, i*4+4);
				break;
			case 2: // Float register
				InstImm( "stfs", PPC_STFS, opStackFloatRegisters[i], R_OPSTACK, i*4+4);
				break;
		}
		opStackRegType[i] = 0;
	}
}

static void loadOpStack(int depth)
{
	// Back off operand stack pointer and reload all operands.
//	InstImm( "addi", PPC_ADDI, R_OPSTACK, R_OPSTACK, -(depth)*4 );

	int i;
	
	for(i = 0; i < depth; i++)
	{
		assert(opStackRegType[i] == 0);
		// For now we're stuck reloading everything as an integer.
		opStackLoadInstructionAddr[i] = &buf[compiledOfs];
		InstImm( "lwz", PPC_LWZ, opStackIntRegisters[i], R_OPSTACK, i*4+4);
		opStackRegType[i] = 1;
	}	
}

static void makeInteger(int depth)
{
	// This should really never be necessary...
	assert(opStackRegType[depth] == 1);
	//assert(opStackRegType[depth] == 2);
	if(opStackRegType[depth] == 2)
	{
		unsigned instruction;
		assert(opStackLoadInstructionAddr[depth]);
		
		printf("patching float load at %p to int load\n",opStackLoadInstructionAddr[depth]);
		// Repatch load instruction to use LFS instead of LWZ
		instruction = *opStackLoadInstructionAddr[depth];
		instruction &= ~PPC_LFSX;
		instruction |=  PPC_LWZX;
		*opStackLoadInstructionAddr[depth] = instruction;
		opStackLoadInstructionAddr[depth] = 0;
		opStackRegType[depth] = 1;
		#if 0
		InstImm( "stfs", PPC_STFS, opStackFloatRegisters[depth], R_OPSTACK, depth*4+4);
		// For XXX make sure we force enough NOPs to get the load into
		// another dispatch group to avoid pipeline flush.
		Inst( "ori", PPC_ORI,  0, 0, 0 );
		Inst( "ori", PPC_ORI,  0, 0, 0 );
		Inst( "ori", PPC_ORI,  0, 0, 0 );
		Inst( "ori", PPC_ORI,  0, 0, 0 );
		InstImm( "lwz", PPC_LWZ, opStackIntRegisters[depth], R_OPSTACK, depth*4+4);
		opStackRegType[depth] = 1;
		#endif
	}
}

static void makeFloat(int depth)
{
	//assert(opStackRegType[depth] == 1);
	if(opStackRegType[depth] == 1)
	{
		unsigned instruction;
		unsigned destReg, aReg, bReg, imm;
		
		if(opStackLoadInstructionAddr[depth])
		{
			// Repatch load instruction to use LFS instead of LWZ
			instruction = *opStackLoadInstructionAddr[depth];
			// Figure out if it's LWZ or LWZX
			if((instruction & 0xfc000000) == PPC_LWZ)
			{
				//printf("patching LWZ at %p to LFS at depth %ld\n",opStackLoadInstructionAddr[depth],depth);
				//printf("old instruction: %08lx\n",instruction);
				// Extract registers
				destReg = (instruction >> 21) & 31;
				aReg    = (instruction >> 16) & 31;
				imm     = instruction & 0xffff;
				
				// Calculate correct FP register to use.
				// THIS ASSUMES REGISTER USAGE FOR THE STACK IS n, n+1, n+2, etc!
				//printf("old dest: %ld\n",destReg);
				destReg = (destReg - opStackIntRegisters[0]) + opStackFloatRegisters[0];
				instruction = PPC_LFS | ( destReg << 21 ) | ( aReg << 16 ) | imm ;			
				//printf("new dest: %ld\n",destReg);
				//printf("new instruction: %08lx\n",instruction);
			}
			else
			{
				//printf("patching LWZX at %p to LFSX at depth %ld\n",opStackLoadInstructionAddr[depth],depth);
				//printf("old instruction: %08lx\n",instruction);
				// Extract registers
				destReg = (instruction >> 21) & 31;
				aReg    = (instruction >> 16) & 31;
				bReg    = (instruction >> 11) & 31;
				// Calculate correct FP register to use.
				// THIS ASSUMES REGISTER USAGE FOR THE STACK IS n, n+1, n+2, etc!
				//printf("old dest: %ld\n",destReg);
				destReg = (destReg - opStackIntRegisters[0]) + opStackFloatRegisters[0];
				instruction = PPC_LFSX | ( destReg << 21 ) | ( aReg << 16 ) | ( bReg << 11 ) ;			
				//printf("new dest: %ld\n",destReg);
				//printf("new instruction: %08lx\n",instruction);
			}
			*opStackLoadInstructionAddr[depth] = instruction;
			opStackLoadInstructionAddr[depth] = 0;
		}
		else
		{	
			//printf("doing float constant load at %p for depth %ld\n",&buf[compiledOfs],depth);
			// It was likely loaded as a constant so we have to save/load it.  A more
			// interesting implementation might be to generate code to do a "PC relative"
			// load from the VM code region.
			InstImm( "stw", PPC_STW, opStackIntRegisters[depth], R_OPSTACK, depth*4+4);
			// For XXX make sure we force enough NOPs to get the load into
			// another dispatch group to avoid pipeline flush.
			Inst( "ori", PPC_ORI,  0, 0, 0 );
			Inst( "ori", PPC_ORI,  0, 0, 0 );
			Inst( "ori", PPC_ORI,  0, 0, 0 );
			Inst( "ori", PPC_ORI,  0, 0, 0 );
			InstImm( "lfs", PPC_LFS, opStackFloatRegisters[depth], R_OPSTACK, depth*4+4);
		}
		opStackRegType[depth] = 2;
	}
}

// TJW: Unused
#if 0
static void fltop() {
    if (rtopped == qfalse) {
	InstImm( PPC_LFS, R_TOP, R_OPSTACK, 0 );		// get value from opstack
    }
}
#endif

#if 0
static void fltopandsecond() {
	InstImm( PPC_LFS, R_TOP, R_OPSTACK, 0 );		// get value from opstack
	InstImm( PPC_LFS, R_SECOND, R_OPSTACK, -4 );	// get value from opstack
	InstImm( PPC_ADDI, R_OPSTACK, R_OPSTACK, -8 );
    rtopped = qfalse;
	return;
}
#endif

#define assertInteger(depth)	assert(opStackRegType[depth] == 1)

/*
=================
VM_Compile
=================
*/
void VM_Compile( vm_t *vm, vmHeader_t *header ) {
	int		op;
	int		maxLength;
	int		v;
	int		i;
        int		opStackDepth;
	
	int		mainFunction;
	
	// set up the into-to-float variables
   	((int *)itofConvert)[0] = 0x43300000;
   	((int *)itofConvert)[1] = 0x80000000;
   	((int *)itofConvert)[2] = 0x43300000;

	// allocate a very large temp buffer, we will shrink it later
	maxLength = header->codeLength * 8;
	buf = Z_Malloc( maxLength );
	jused = Z_Malloc(header->instructionCount + 2);
	Com_Memset(jused, 0, header->instructionCount+2);
	
    // compile everything twice, so the second pass will have valid instruction
    // pointers for branches
    for ( pass = -1 ; pass < 2 ; pass++ ) {

        // translate all instructions
        pc = 0;
	mainFunction = 0;
	opStackDepth = 0;
	
	pop0 = 343545;
	pop1 = 2443545;
	oc0 = -2343535;
	oc1 = 24353454;
	tvm = vm;
        code = (byte *)header + header->codeOffset;
        compiledOfs = 0;
#ifndef __GNUC__
		// metrowerks seems to require this header in front of functions
		Emit4( (int)(buf+2) );
		Emit4( 0 );
#endif

	for ( instruction = 0 ; instruction < header->instructionCount ; instruction++ ) {
            if ( compiledOfs*4 > maxLength - 16 ) {
                Com_Error( ERR_DROP, "VM_Compile: maxLength exceeded" );
            }

            op = code[ pc ];
            if ( !pass ) {
                vm->instructionPointers[ instruction ] = compiledOfs * 4;
            }
            pc++;
            switch ( op ) {
            case 0:
                break;
            case OP_BREAK:
		#if DEBUG_VM
		if(pass == 1)
			printf("%08lx BREAK\n",instruction);
		#endif
                InstImmU( "addi", PPC_ADDI, R_TOP, 0, 0 );
                InstImm( "lwz", PPC_LWZ, R_TOP, R_TOP, 0 );			// *(int *)0 to crash to debugger
                break;
            case OP_ENTER:
		opStackDepth = 0;
		v = Constant4();
		#if DEBUG_VM
		if(pass == 1)
			printf("%08x ENTER\t%04x\n",instruction,v);
		#endif
		opStackRegType[opStackDepth] = 0;
		mainFunction++;
		if(mainFunction == 1)
		{
			// Main VM entry point is the first thing we compile, so save off operand stack
			// registers here.  This avoids issues with trying to trick the native compiler
			// into doing it, and properly matches the PowerPC ABI
			InstImm( "addi", PPC_ADDI, R_REAL_STACK, R_REAL_STACK, -OP_STACK_MAX_DEPTH*4 );	// sub R_STACK, R_STACK, imm
			for(i = 0; i < OP_STACK_MAX_DEPTH; i++)
				InstImm( "stw", PPC_STW, opStackIntRegisters[i], R_REAL_STACK, i*4);
		}
                InstImm( "addi", PPC_ADDI, R_STACK, R_STACK, -v );	// sub R_STACK, R_STACK, imm
                break;
            case OP_CONST:
                v = Constant4();
		#if DEBUG_VM
		if(pass == 1)
			printf("%08x CONST\t%08x\n",instruction,v);
		#endif
		opStackLoadInstructionAddr[opStackDepth] = 0;
                if ( v < 32768 && v >= -32768 ) {
                    InstImmU( "addi", PPC_ADDI, opStackIntRegisters[opStackDepth], 0, v & 0xffff );
                } else {
                    InstImmU( "addis", PPC_ADDIS, opStackIntRegisters[opStackDepth], 0, (v >> 16)&0xffff );
                    if ( v & 0xffff ) {
                        InstImmU( "ori", PPC_ORI, opStackIntRegisters[opStackDepth], opStackIntRegisters[opStackDepth], v & 0xffff );
                    }
                }
		opStackRegType[opStackDepth] = 1;
		opStackDepth += 1;
		if (code[pc] == OP_JUMP) {
		    jused[v] = 1;
		}
		break;
            case OP_LOCAL:
		oc1 = Constant4();
		#if DEBUG_VM
		if(pass == 1)
			printf("%08x LOCAL\t%08x\n",instruction,oc1);
		#endif
		if (code[pc] == OP_LOAD4 || code[pc] == OP_LOAD2 || code[pc] == OP_LOAD1) {
		    oc1 &= vm->dataMask;
		}
                InstImm( "addi", PPC_ADDI, opStackIntRegisters[opStackDepth], R_STACK, oc1 );
		opStackRegType[opStackDepth] = 1;
		opStackLoadInstructionAddr[opStackDepth] = 0;
		opStackDepth += 1;
                break;
            case OP_ARG:
		v = Constant1();
		#if DEBUG_VM
		if(pass == 1)
			printf("%08x ARG \t%08x\n",instruction,v);
		#endif
                InstImm( "addi", PPC_ADDI, R_EA, R_STACK, v );	// location to put it
		if(opStackRegType[opStackDepth-1] == 1)
			Inst( "stwx", PPC_STWX, opStackIntRegisters[opStackDepth-1], R_EA, R_MEMBASE );
		else
			Inst( "stfsx", PPC_STFSX, opStackFloatRegisters[opStackDepth-1], R_EA, R_MEMBASE );
		opStackRegType[opStackDepth-1] = 0;
		opStackLoadInstructionAddr[opStackDepth-1] = 0;
		opStackDepth -= 1;
		
                break;
            case OP_CALL:
		#if DEBUG_VM
		if(pass == 1)
			printf("%08x CALL\n",instruction);
		#endif
		assertInteger(opStackDepth-1);
		assert(opStackDepth > 0);
                Inst( "mflr", PPC_MFSPR, R_SECOND, 8, 0 );			// move from link register
                InstImm( "stwu", PPC_STWU, R_SECOND, R_REAL_STACK, -16 );	// save off the old return address

		// Spill operand stack registers.
		spillOpStack(opStackDepth);
		
		// We need to leave R_OPSTACK pointing to the top entry on the stack, which is the call address.
		// It will be consumed (and R4 decremented) by the AsmCall code.
		InstImm( "addi", PPC_ADDI, R_OPSTACK, R_OPSTACK, opStackDepth*4);

                Inst( "mtctr", PPC_MTSPR, R_ASMCALL, 9, 0 );			// move to count register
                Inst( "bctrl", PPC_BCCTR | 1, 20, 0, 0 );			// jump and link to the count register

		// R4 now points to the top of the operand stack, which has the return value in it.  We want to
		// back off the pointer to point to the base of our local operand stack and then reload the stack.
		
		InstImm("addi", PPC_ADDI, R_OPSTACK, R_OPSTACK, -opStackDepth*4);
		
		// Reload operand stack.
		loadOpStack(opStackDepth);

                InstImm( "lwz", PPC_LWZ, R_SECOND, R_REAL_STACK, 0 );		// fetch the old return address
                InstImm( "addi", PPC_ADDI, R_REAL_STACK, R_REAL_STACK, 16 );
                Inst( "mtlr", PPC_MTSPR, R_SECOND, 8, 0 );			// move to link register
                break;
            case OP_PUSH:
		#if DEBUG_VM
		if(pass == 1)
			printf("%08x PUSH\n",instruction);
		#endif
		opStackRegType[opStackDepth] = 1; 	// Garbage int value.
		opStackDepth += 1;
                break;
            case OP_POP:
		#if DEBUG_VM
		if(pass == 1)
			printf("%08x POP\n",instruction);
		#endif
		opStackDepth -= 1;
		opStackRegType[opStackDepth] = 0; 	// ??
		opStackLoadInstructionAddr[opStackDepth-1] = 0;
                break;
            case OP_LEAVE:
		#if DEBUG_VM
		if(pass == 1)
			printf("%08x LEAVE\n",instruction);
		#endif
		assert(opStackDepth == 1);
		assert(opStackRegType[0] != 0);
		// Save return value onto top of op stack.  We also have to increment R_OPSTACK
		switch(opStackRegType[0])
		{
			case 1:	// Integer register
				InstImm( "stw", PPC_STWU, opStackIntRegisters[0], R_OPSTACK, 4);
				break;
			case 2: // Float register
				InstImm( "stfs", PPC_STFSU, opStackFloatRegisters[0], R_OPSTACK, 4);
				break;
		}
                InstImm( "addi", PPC_ADDI, R_STACK, R_STACK, Constant4() );	// add R_STACK, R_STACK, imm
		if(mainFunction == 1)
		{
			for(i = 0; i < OP_STACK_MAX_DEPTH; i++)
				InstImm( "lwz", PPC_LWZ,  opStackIntRegisters[i], R_REAL_STACK, i*4);
			InstImm( "addi", PPC_ADDI, R_REAL_STACK, R_REAL_STACK, OP_STACK_MAX_DEPTH*4 );	
		}
		opStackDepth--;
		opStackRegType[opStackDepth] = 0;
		opStackLoadInstructionAddr[opStackDepth] = 0;
                Inst( "blr", PPC_BCLR, 20, 0, 0 );							// branch unconditionally to link register
                break;
            case OP_LOAD4:
		#if DEBUG_VM
		if(pass == 1)
			printf("%08x LOAD4\n",instruction);
		#endif
		// We should try to figure out whether to use LWZX or LFSX based
		// on some kind of code analysis after subsequent passes.  I think what
		// we could do is store the compiled load instruction address along with
		// the register type.  When we hit the first mismatched operator, we go back
		// and patch the load.  Since LCC's operand stack should be at 0 depth by the
		// time we hit a branch, this should work fairly well.  FIXME FIXME FIXME.
		assertInteger(opStackDepth-1);
		opStackLoadInstructionAddr[opStackDepth-1] = &buf[ compiledOfs ];
                Inst( "lwzx", PPC_LWZX, opStackIntRegisters[opStackDepth-1], opStackIntRegisters[opStackDepth-1], R_MEMBASE );// load from memory base
		opStackRegType[opStackDepth-1] = 1;
                break;
            case OP_LOAD2:
		#if DEBUG_VM
		if(pass == 1)
			printf("%08x LOAD2\n",instruction);
		#endif
		assertInteger(opStackDepth-1);
		opStackLoadInstructionAddr[opStackDepth-1] = 0;
                Inst( "lhzx", PPC_LHZX, opStackIntRegisters[opStackDepth-1], opStackIntRegisters[opStackDepth-1], R_MEMBASE );// load from memory base
		opStackRegType[opStackDepth-1] = 1;
                break;
            case OP_LOAD1:
		#if DEBUG_VM
		if(pass == 1)
			printf("%08x LOAD1\n",instruction);
		#endif
		assertInteger(opStackDepth-1);
		opStackLoadInstructionAddr[opStackDepth-1] = 0;
                Inst( "lbzx", PPC_LBZX, opStackIntRegisters[opStackDepth-1], opStackIntRegisters[opStackDepth-1], R_MEMBASE );// load from memory base
		opStackRegType[opStackDepth-1] = 1;
                break;
            case OP_STORE4:
		#if DEBUG_VM
		if(pass == 1)
			printf("%08x STORE4\n",instruction);
		#endif
		assertInteger(opStackDepth-2);
		if(opStackRegType[opStackDepth-1] == 1)
			Inst( "stwx", PPC_STWX, opStackIntRegisters[opStackDepth-1], 
					opStackIntRegisters[opStackDepth-2], R_MEMBASE );		// store from memory base
		else
			Inst( "stfsx", PPC_STFSX, opStackFloatRegisters[opStackDepth-1], 
					opStackIntRegisters[opStackDepth-2], R_MEMBASE );		// store from memory base
		opStackRegType[opStackDepth-1] = 0;
		opStackRegType[opStackDepth-2] = 0;
		opStackLoadInstructionAddr[opStackDepth-1] = 0;
		opStackLoadInstructionAddr[opStackDepth-2] = 0;
		opStackDepth -= 2;
                break;
            case OP_STORE2:
		#if DEBUG_VM
		if(pass == 1)
			printf("%08x STORE2\n",instruction);
		#endif
		assertInteger(opStackDepth-1);
		assertInteger(opStackDepth-2);
                Inst( "sthx", PPC_STHX, opStackIntRegisters[opStackDepth-1],
				opStackIntRegisters[opStackDepth-2], R_MEMBASE );		// store from memory base
		opStackRegType[opStackDepth-1] = 0;
		opStackRegType[opStackDepth-2] = 0;
		opStackLoadInstructionAddr[opStackDepth-1] = 0;
		opStackLoadInstructionAddr[opStackDepth-2] = 0;
		opStackDepth -= 2;
                break;
            case OP_STORE1:
		#if DEBUG_VM
		if(pass == 1)
			printf("%08x STORE1\n",instruction);
		#endif
		assertInteger(opStackDepth-1);
		assertInteger(opStackDepth-2);
                Inst( "stbx", PPC_STBX, opStackIntRegisters[opStackDepth-1],
				opStackIntRegisters[opStackDepth-2], R_MEMBASE );		// store from memory base
		opStackRegType[opStackDepth-1] = 0;
		opStackRegType[opStackDepth-2] = 0;
		opStackLoadInstructionAddr[opStackDepth-1] = 0;
		opStackLoadInstructionAddr[opStackDepth-2] = 0;
		opStackDepth -= 2;
                break;

            case OP_EQ:
		#if DEBUG_VM
		if(pass == 1)
			printf("%08x EQ\n",instruction);
		#endif
		assertInteger(opStackDepth-1);
		assertInteger(opStackDepth-2);
                Inst( "cmp", PPC_CMP, 0, opStackIntRegisters[opStackDepth-2], opStackIntRegisters[opStackDepth-1] );
		opStackRegType[opStackDepth-1] = 0;
		opStackRegType[opStackDepth-2] = 0;
		opStackLoadInstructionAddr[opStackDepth-1] = 0;
		opStackLoadInstructionAddr[opStackDepth-2] = 0;
		opStackDepth -= 2;
                i = Constant4();
				jused[i] = 1;
                InstImm( "bc", PPC_BC, 4, 2, 8 );
                if ( pass==1 ) {
                    v = vm->instructionPointers[ i ] - (int)&buf[compiledOfs];                    
                } else {
                    v = 0;             
                }
                Emit4("b", PPC_B | (v&0x3ffffff) );
                break;
            case OP_NE:
		#if DEBUG_VM
		if(pass == 1)
			printf("%08x NE\n",instruction);
		#endif
		assertInteger(opStackDepth-1);
		assertInteger(opStackDepth-2);
                Inst( "cmp", PPC_CMP, 0, opStackIntRegisters[opStackDepth-2], opStackIntRegisters[opStackDepth-1] );
		opStackRegType[opStackDepth-1] = 0;
		opStackRegType[opStackDepth-2] = 0;
		opStackLoadInstructionAddr[opStackDepth-1] = 0;
		opStackLoadInstructionAddr[opStackDepth-2] = 0;
		opStackDepth -= 2;
                i = Constant4();
				jused[i] = 1;
                InstImm( "bc", PPC_BC, 12, 2, 8 );
                if ( pass==1 ) {
                    v = vm->instructionPointers[ i ] - (int)&buf[compiledOfs];                    
                } else {
                    v = 0;
                }
                Emit4("b", PPC_B | (unsigned int)(v&0x3ffffff) );
//                InstImm( "bc", PPC_BC, 4, 2, v );

                break;
            case OP_LTI:
		#if DEBUG_VM
		if(pass == 1)
		printf("%08x LTI\n",instruction);
		#endif
		assertInteger(opStackDepth-1);
		assertInteger(opStackDepth-2);
                Inst( "cmp", PPC_CMP, 0, opStackIntRegisters[opStackDepth-2], opStackIntRegisters[opStackDepth-1] );
		opStackRegType[opStackDepth-1] = 0;
		opStackRegType[opStackDepth-2] = 0;
		opStackLoadInstructionAddr[opStackDepth-1] = 0;
		opStackLoadInstructionAddr[opStackDepth-2] = 0;
		opStackDepth -= 2;
                i = Constant4();
				jused[i] = 1;
                InstImm( "bc", PPC_BC, 4, 0, 8 );
                if ( pass==1 ) {
                    v = vm->instructionPointers[ i ] - (int)&buf[compiledOfs];
                } else {
                    v = 0;
                }
                Emit4("b", PPC_B | (unsigned int)(v&0x3ffffff) );
//                InstImm( "bc", PPC_BC, 12, 0, v );
                break;
            case OP_LEI:
		#if DEBUG_VM
		if(pass == 1)
		printf("%08x LEI\n",instruction);
		#endif
		assertInteger(opStackDepth-1);
		assertInteger(opStackDepth-2);
                Inst( "cmp", PPC_CMP, 0, opStackIntRegisters[opStackDepth-2], opStackIntRegisters[opStackDepth-1] );
		opStackRegType[opStackDepth-1] = 0;
		opStackRegType[opStackDepth-2] = 0;
		opStackLoadInstructionAddr[opStackDepth-1] = 0;
		opStackLoadInstructionAddr[opStackDepth-2] = 0;
		opStackDepth -= 2;
                i = Constant4();
				jused[i] = 1;
                InstImm( "bc", PPC_BC, 12, 1, 8 );
                if ( pass==1 ) {
                    v = vm->instructionPointers[ i ] - (int)&buf[compiledOfs];
                } else {
                    v = 0;
                }
                Emit4("b", PPC_B | (unsigned int)(v&0x3ffffff) );
//                InstImm( "bc", PPC_BC, 4, 1, v );
                break;
            case OP_GTI:
		#if DEBUG_VM
		if(pass == 1)
		printf("%08x GTI\n",instruction);
		#endif
		assertInteger(opStackDepth-1);
		assertInteger(opStackDepth-2);
                Inst( "cmp", PPC_CMP, 0, opStackIntRegisters[opStackDepth-2], opStackIntRegisters[opStackDepth-1] );
		opStackRegType[opStackDepth-1] = 0;
		opStackRegType[opStackDepth-2] = 0;
		opStackLoadInstructionAddr[opStackDepth-1] = 0;
		opStackLoadInstructionAddr[opStackDepth-2] = 0;
		opStackDepth -= 2;
                i = Constant4();
				jused[i] = 1;
                InstImm( "bc", PPC_BC, 4, 1, 8 );
                if ( pass==1 ) {
                    v = vm->instructionPointers[ i ] - (int)&buf[compiledOfs];
                } else {
                    v = 0;
                }
                Emit4("b", PPC_B | (unsigned int)(v&0x3ffffff) );
//                InstImm( "bc", PPC_BC, 12, 1, v );
                break;
            case OP_GEI:
		#if DEBUG_VM
		if(pass == 1)
		printf("%08x GEI\n",instruction);
		#endif
		assertInteger(opStackDepth-1);
		assertInteger(opStackDepth-2);
                Inst( "cmp", PPC_CMP, 0, opStackIntRegisters[opStackDepth-2], opStackIntRegisters[opStackDepth-1] );
		opStackRegType[opStackDepth-1] = 0;
		opStackRegType[opStackDepth-2] = 0;
		opStackLoadInstructionAddr[opStackDepth-1] = 0;
		opStackLoadInstructionAddr[opStackDepth-2] = 0;
		opStackDepth -= 2;
                i = Constant4();
				jused[i] = 1;
                InstImm( "bc", PPC_BC, 12, 0, 8 );
                if ( pass==1 ) {
                    v = vm->instructionPointers[ i ] - (int)&buf[compiledOfs];
                } else {
                    v = 0;
                }
                Emit4("b", PPC_B | (unsigned int)(v&0x3ffffff) );
//                InstImm( "bc", PPC_BC, 4, 0, v );
                break;
            case OP_LTU:
		#if DEBUG_VM
		if(pass == 1)
		printf("%08x LTU\n",instruction);
		#endif
		assertInteger(opStackDepth-1);
		assertInteger(opStackDepth-2);
                Inst( "cmp", PPC_CMP, 0, opStackIntRegisters[opStackDepth-2], opStackIntRegisters[opStackDepth-1] );
		opStackRegType[opStackDepth-1] = 0;
		opStackRegType[opStackDepth-2] = 0;
		opStackLoadInstructionAddr[opStackDepth-1] = 0;
		opStackLoadInstructionAddr[opStackDepth-2] = 0;
		opStackDepth -= 2;
                i = Constant4();
		jused[i] = 1;
                InstImm( "bc", PPC_BC, 4, 0, 8 );
                if ( pass==1 ) {
                    v = vm->instructionPointers[ i ] - (int)&buf[compiledOfs];
                } else {
                    v = 0;
                }
                Emit4("b", PPC_B | (unsigned int)(v&0x3ffffff) );
//                InstImm( "bc", PPC_BC, 12, 0, v );
                break;
            case OP_LEU:
		#if DEBUG_VM
		if(pass == 1)
		printf("%08x LEU\n",instruction);
		#endif
		assertInteger(opStackDepth-1);
		assertInteger(opStackDepth-2);
                Inst( "cmp", PPC_CMP, 0, opStackIntRegisters[opStackDepth-2], opStackIntRegisters[opStackDepth-1] );
		opStackRegType[opStackDepth-1] = 0;
		opStackRegType[opStackDepth-2] = 0;
		opStackLoadInstructionAddr[opStackDepth-1] = 0;
		opStackLoadInstructionAddr[opStackDepth-2] = 0;
		opStackDepth -= 2;
                i = Constant4();
		jused[i] = 1;
                InstImm( "bc", PPC_BC, 12, 1, 8 );
                if ( pass==1 ) {
                    v = vm->instructionPointers[ i ] - (int)&buf[compiledOfs];
                } else {
                    v = 0;
                }
                Emit4("b", PPC_B | (unsigned int)(v&0x3ffffff) );
//                InstImm( "bc", PPC_BC, 4, 1, v );
                break;
            case OP_GTU:
		#if DEBUG_VM
		if(pass == 1)
		printf("%08x GTU\n",instruction);
		#endif
		assertInteger(opStackDepth-1);
		assertInteger(opStackDepth-2);
                Inst( "cmp", PPC_CMP, 0, opStackIntRegisters[opStackDepth-2], opStackIntRegisters[opStackDepth-1] );
		opStackRegType[opStackDepth-1] = 0;
		opStackRegType[opStackDepth-2] = 0;
		opStackLoadInstructionAddr[opStackDepth-1] = 0;
		opStackLoadInstructionAddr[opStackDepth-2] = 0;
		opStackDepth -= 2;
                i = Constant4();
		jused[i] = 1;
                InstImm( "bc", PPC_BC, 4, 1, 8 );
                if ( pass==1 ) {
                    v = vm->instructionPointers[ i ] - (int)&buf[compiledOfs];
                } else {
                    v = 0;
                }
                Emit4("b", PPC_B | (unsigned int)(v&0x3ffffff) );
//                InstImm( "bc", PPC_BC, 12, 1, v );
                break;
            case OP_GEU:
		#if DEBUG_VM
		if(pass == 1)
		printf("%08x GEU\n",instruction);
		#endif
		assertInteger(opStackDepth-1);
		assertInteger(opStackDepth-2);
                Inst( "cmp", PPC_CMP, 0, opStackIntRegisters[opStackDepth-2], opStackIntRegisters[opStackDepth-1] );
		opStackRegType[opStackDepth-1] = 0;
		opStackRegType[opStackDepth-2] = 0;
		opStackLoadInstructionAddr[opStackDepth-1] = 0;
		opStackLoadInstructionAddr[opStackDepth-2] = 0;
		opStackDepth -= 2;
                i = Constant4();
		jused[i] = 1;
                InstImm( "bc", PPC_BC, 12, 0, 8 );
                if ( pass==1 ) {
                    v = vm->instructionPointers[ i ] - (int)&buf[compiledOfs];
                } else {
                    v = 0;
                }
                Emit4("b", PPC_B | (unsigned int)(v&0x3ffffff) );
//                InstImm( "bc", PPC_BC, 4, 0, v );
                break;
                
            case OP_EQF:
		#if DEBUG_VM
		if(pass == 1)
		printf("%08x EQF\n",instruction);
		#endif
		makeFloat(opStackDepth-1);
		makeFloat(opStackDepth-2);
                Inst( "fcmpu", PPC_FCMPU, 0, opStackFloatRegisters[opStackDepth-2], opStackFloatRegisters[opStackDepth-1] );
		opStackRegType[opStackDepth-1] = 0;
		opStackRegType[opStackDepth-2] = 0;
		opStackLoadInstructionAddr[opStackDepth-1] = 0;
		opStackLoadInstructionAddr[opStackDepth-2] = 0;
		opStackDepth -= 2;
                i = Constant4();
		jused[i] = 1;
                InstImm( "bc", PPC_BC, 4, 2, 8 );
                if ( pass==1 ) {
                    v = vm->instructionPointers[ i ] - (int)&buf[compiledOfs];
                } else {
                    v = 0;
                }
                Emit4("b", PPC_B | (unsigned int)(v&0x3ffffff) );
//                InstImm( "bc", PPC_BC, 12, 2, v );
                break;			
            case OP_NEF:
		#if DEBUG_VM
		if(pass == 1)
		printf("%08x NEF\n",instruction);
		#endif
		makeFloat(opStackDepth-1);
		makeFloat(opStackDepth-2);
                Inst( "fcmpu", PPC_FCMPU, 0, opStackFloatRegisters[opStackDepth-2], opStackFloatRegisters[opStackDepth-1] );
		opStackRegType[opStackDepth-1] = 0;
		opStackRegType[opStackDepth-2] = 0;
		opStackLoadInstructionAddr[opStackDepth-1] = 0;
		opStackLoadInstructionAddr[opStackDepth-2] = 0;
		opStackDepth -= 2;
                i = Constant4();
		jused[i] = 1;
                InstImm( "bc", PPC_BC, 12, 2, 8 );
                if ( pass==1 ) {
                    v = vm->instructionPointers[ i ] - (int)&buf[compiledOfs];
                } else {
                    v = 0;
                }
                Emit4("b", PPC_B | (unsigned int)(v&0x3ffffff) );
//                InstImm( "bc", PPC_BC, 4, 2, v );
                break;			
            case OP_LTF:
		#if DEBUG_VM
		if(pass == 1)
		printf("%08x LTF\n",instruction);
		#endif
		makeFloat(opStackDepth-1);
		makeFloat(opStackDepth-2);
                Inst( "fcmpu", PPC_FCMPU, 0, opStackFloatRegisters[opStackDepth-2], opStackFloatRegisters[opStackDepth-1] );
		opStackRegType[opStackDepth-1] = 0;
		opStackRegType[opStackDepth-2] = 0;
		opStackLoadInstructionAddr[opStackDepth-1] = 0;
		opStackLoadInstructionAddr[opStackDepth-2] = 0;
		opStackDepth -= 2;
                i = Constant4();
		jused[i] = 1;
                InstImm( "bc", PPC_BC, 4, 0, 8 );
                if ( pass==1 ) {
                    v = vm->instructionPointers[ i ] - (int)&buf[compiledOfs];
                } else {
                    v = 0;
                }
                Emit4("b", PPC_B | (unsigned int)(v&0x3ffffff) );
//                InstImm( "bc", PPC_BC, 12, 0, v );
                break;			
            case OP_LEF:
		#if DEBUG_VM
		if(pass == 1)
		printf("%08x LEF\n",instruction);
		#endif
		makeFloat(opStackDepth-1);
		makeFloat(opStackDepth-2);
                Inst( "fcmpu", PPC_FCMPU, 0, opStackFloatRegisters[opStackDepth-2], opStackFloatRegisters[opStackDepth-1] );
		opStackRegType[opStackDepth-1] = 0;
		opStackRegType[opStackDepth-2] = 0;
		opStackLoadInstructionAddr[opStackDepth-1] = 0;
		opStackLoadInstructionAddr[opStackDepth-2] = 0;
		opStackDepth -= 2;
                i = Constant4();
		jused[i] = 1;
                InstImm( "bc", PPC_BC, 12, 1, 8 );
                if ( pass==1 ) {
                    v = vm->instructionPointers[ i ] - (int)&buf[compiledOfs];
                } else {
                    v = 0;
                }
                Emit4("b", PPC_B | (unsigned int)(v&0x3ffffff) );
//                InstImm( "bc", PPC_BC, 4, 1, v );
                break;			
            case OP_GTF:
		#if DEBUG_VM
		if(pass == 1)
		printf("%08x GTF\n",instruction);
		#endif
		makeFloat(opStackDepth-1);
		makeFloat(opStackDepth-2);
                Inst( "fcmpu", PPC_FCMPU, 0, opStackFloatRegisters[opStackDepth-2], opStackFloatRegisters[opStackDepth-1] );
		opStackRegType[opStackDepth-1] = 0;
		opStackRegType[opStackDepth-2] = 0;
		opStackLoadInstructionAddr[opStackDepth-1] = 0;
		opStackLoadInstructionAddr[opStackDepth-2] = 0;
		opStackDepth -= 2;
                i = Constant4();
		jused[i] = 1;
                InstImm( "bc", PPC_BC, 4, 1, 8 );
                if ( pass==1 ) {
                    v = vm->instructionPointers[ i ] - (int)&buf[compiledOfs];
                } else {
                    v = 0;
                }
                Emit4("b", PPC_B | (unsigned int)(v&0x3ffffff) );
//                InstImm( "bc", PPC_BC, 12, 1, v );
                break;			
            case OP_GEF:
		#if DEBUG_VM
		if(pass == 1)
		printf("%08x GEF\n",instruction);
		#endif
		makeFloat(opStackDepth-1);
		makeFloat(opStackDepth-2);
                Inst( "fcmpu", PPC_FCMPU, 0, opStackFloatRegisters[opStackDepth-2], opStackFloatRegisters[opStackDepth-1] );
		opStackRegType[opStackDepth-1] = 0;
		opStackRegType[opStackDepth-2] = 0;
		opStackLoadInstructionAddr[opStackDepth-1] = 0;
		opStackLoadInstructionAddr[opStackDepth-2] = 0;
		opStackDepth -= 2;
                i = Constant4();
		jused[i] = 1;
                InstImm( "bc", PPC_BC, 12, 0, 8 );
                if ( pass==1 ) {
                    v = vm->instructionPointers[ i ] - (int)&buf[compiledOfs];
                } else {
                    v = 0;
                }
                Emit4("b", PPC_B | (unsigned int)(v&0x3ffffff) );
//                InstImm( "bc", PPC_BC, 4, 0, v );
                break;

            case OP_NEGI:
		#if DEBUG_VM
		if(pass == 1)
		printf("%08x NEGI\n",instruction);
		#endif
		assertInteger(opStackDepth-1);
                InstImm( "subfic", PPC_SUBFIC, opStackIntRegisters[opStackDepth-1], opStackIntRegisters[opStackDepth-1], 0 );
		opStackLoadInstructionAddr[opStackDepth-1] = 0;
                break;
            case OP_ADD:
		#if DEBUG_VM
		if(pass == 1)
		printf("%08x ADD\n",instruction);
		#endif
		assertInteger(opStackDepth-1);
		assertInteger(opStackDepth-2);
                Inst( "add", PPC_ADD, opStackIntRegisters[opStackDepth-2], opStackIntRegisters[opStackDepth-1], opStackIntRegisters[opStackDepth-2] );
		opStackRegType[opStackDepth-1] = 0;
		opStackLoadInstructionAddr[opStackDepth-1] = 0;
		opStackDepth -= 1;
                break;
            case OP_SUB:
		#if DEBUG_VM
		if(pass == 1)
		printf("%08x SUB\n",instruction);
		#endif
		assertInteger(opStackDepth-1);
		assertInteger(opStackDepth-2);
                Inst( "subf", PPC_SUBF, opStackIntRegisters[opStackDepth-2], opStackIntRegisters[opStackDepth-1], opStackIntRegisters[opStackDepth-2] );
		opStackRegType[opStackDepth-1] = 0;
		opStackLoadInstructionAddr[opStackDepth-1] = 0;
		opStackDepth -= 1;
                break;
            case OP_DIVI:
		#if DEBUG_VM
		if(pass == 1)
		printf("%08x DIVI\n",instruction);
		#endif
		assertInteger(opStackDepth-1);
		assertInteger(opStackDepth-2);
                Inst( "divw", PPC_DIVW, opStackIntRegisters[opStackDepth-2], opStackIntRegisters[opStackDepth-2], opStackIntRegisters[opStackDepth-1] );
		opStackRegType[opStackDepth-1] = 0;
		opStackLoadInstructionAddr[opStackDepth-1] = 0;
		opStackDepth -= 1;
                break;
            case OP_DIVU:
		#if DEBUG_VM
		if(pass == 1)
		printf("%08x DIVU\n",instruction);
		#endif
		assertInteger(opStackDepth-1);
		assertInteger(opStackDepth-2);
                Inst( "divwu", PPC_DIVWU, opStackIntRegisters[opStackDepth-2], opStackIntRegisters[opStackDepth-2], opStackIntRegisters[opStackDepth-1] );
		opStackRegType[opStackDepth-1] = 0;
		opStackLoadInstructionAddr[opStackDepth-1] = 0;
		opStackDepth -= 1;
                break;
            case OP_MODI:
		#if DEBUG_VM
		if(pass == 1)
		printf("%08x MODI\n",instruction);
		#endif
		assertInteger(opStackDepth-1);
		assertInteger(opStackDepth-2);
                Inst( "divw", PPC_DIVW, R_EA, opStackIntRegisters[opStackDepth-2], opStackIntRegisters[opStackDepth-1] );
                Inst( "mullw", PPC_MULLW, R_EA, opStackIntRegisters[opStackDepth-1], R_EA );
                Inst( "subf", PPC_SUBF, opStackIntRegisters[opStackDepth-2], R_EA, opStackIntRegisters[opStackDepth-2] );
		opStackRegType[opStackDepth-1] = 0;
		opStackLoadInstructionAddr[opStackDepth-1] = 0;
		opStackDepth -= 1;
                break;
            case OP_MODU:
		#if DEBUG_VM
		if(pass == 1)
		printf("%08x MODU\n",instruction);
		#endif
		assertInteger(opStackDepth-1);
		assertInteger(opStackDepth-2);
                Inst( "divwu", PPC_DIVWU, R_EA, opStackIntRegisters[opStackDepth-2], opStackIntRegisters[opStackDepth-1] );
                Inst( "mullw", PPC_MULLW, R_EA, opStackIntRegisters[opStackDepth-1], R_EA );
                Inst( "subf", PPC_SUBF, opStackIntRegisters[opStackDepth-2], R_EA, opStackIntRegisters[opStackDepth-2] );
		opStackRegType[opStackDepth-1] = 0;
		opStackLoadInstructionAddr[opStackDepth-1] = 0;
		opStackDepth -= 1;
                break;
            case OP_MULI:
            case OP_MULU:
		#if DEBUG_VM
		if(pass == 1)
		printf("%08x MULI\n",instruction);
		#endif
		assertInteger(opStackDepth-1);
		assertInteger(opStackDepth-2);
                Inst( "mullw", PPC_MULLW, opStackIntRegisters[opStackDepth-2], opStackIntRegisters[opStackDepth-1], opStackIntRegisters[opStackDepth-2] );
		opStackRegType[opStackDepth-1] = 0;
		opStackLoadInstructionAddr[opStackDepth-1] = 0;
		opStackDepth -= 1;
                break;
            case OP_BAND:
		#if DEBUG_VM
		if(pass == 1)
		printf("%08x BAND\n",instruction);
		#endif
		assertInteger(opStackDepth-1);
		assertInteger(opStackDepth-2);
                Inst( "and", PPC_AND, opStackIntRegisters[opStackDepth-2], opStackIntRegisters[opStackDepth-2], opStackIntRegisters[opStackDepth-1] );
		opStackRegType[opStackDepth-1] = 0;
		opStackLoadInstructionAddr[opStackDepth-1] = 0;
		opStackDepth -= 1;
                break;
            case OP_BOR:
		#if DEBUG_VM
		if(pass == 1)
		printf("%08x BOR\n",instruction);
		#endif
		assertInteger(opStackDepth-1);
		assertInteger(opStackDepth-2);
                Inst( "or", PPC_OR, opStackIntRegisters[opStackDepth-2], opStackIntRegisters[opStackDepth-2], opStackIntRegisters[opStackDepth-1] );
		opStackRegType[opStackDepth-1] = 0;
		opStackLoadInstructionAddr[opStackDepth-1] = 0;
		opStackDepth -= 1;
                break;
            case OP_BXOR:
		#if DEBUG_VM
		if(pass == 1)
		printf("%08x BXOR\n",instruction);
		#endif
		assertInteger(opStackDepth-1);
		assertInteger(opStackDepth-2);
                Inst( "xor", PPC_XOR, opStackIntRegisters[opStackDepth-2], opStackIntRegisters[opStackDepth-2], opStackIntRegisters[opStackDepth-1] );
		opStackRegType[opStackDepth-1] = 0;
		opStackLoadInstructionAddr[opStackDepth-1] = 0;
		opStackDepth -= 1;
                break;
            case OP_BCOM:
		#if DEBUG_VM
		if(pass == 1)
		printf("%08x BCOM\n",instruction);
		#endif
		assertInteger(opStackDepth-1);
                Inst( "nor", PPC_NOR, opStackIntRegisters[opStackDepth-1], opStackIntRegisters[opStackDepth-1], opStackIntRegisters[opStackDepth-1] );
		opStackLoadInstructionAddr[opStackDepth-1] = 0;
                break;
            case OP_LSH:
		#if DEBUG_VM
		if(pass == 1)
		printf("%08x LSH\n",instruction);
		#endif
		assertInteger(opStackDepth-1);
		assertInteger(opStackDepth-2);
                Inst( "slw", PPC_SLW, opStackIntRegisters[opStackDepth-2], opStackIntRegisters[opStackDepth-2], opStackIntRegisters[opStackDepth-1] );
		opStackRegType[opStackDepth-1] = 0;
		opStackLoadInstructionAddr[opStackDepth-1] = 0;
		opStackDepth -= 1;
                break;
            case OP_RSHI:
		#if DEBUG_VM
		if(pass == 1)
		printf("%08x RSHI\n",instruction);
		#endif
		assertInteger(opStackDepth-1);
		assertInteger(opStackDepth-2);
                Inst( "sraw", PPC_SRAW, opStackIntRegisters[opStackDepth-2], opStackIntRegisters[opStackDepth-2], opStackIntRegisters[opStackDepth-1] );
		opStackRegType[opStackDepth-1] = 0;
		opStackLoadInstructionAddr[opStackDepth-1] = 0;
		opStackDepth -= 1;
                break;
            case OP_RSHU:
		#if DEBUG_VM
		if(pass == 1)
		printf("%08x RSHU\n",instruction);
		#endif
		assertInteger(opStackDepth-1);
		assertInteger(opStackDepth-2);
                Inst( "srw", PPC_SRW, opStackIntRegisters[opStackDepth-2], opStackIntRegisters[opStackDepth-2], opStackIntRegisters[opStackDepth-1] );
		opStackRegType[opStackDepth-1] = 0;
		opStackLoadInstructionAddr[opStackDepth-1] = 0;
		opStackDepth -= 1;
                break;

            case OP_NEGF:
		#if DEBUG_VM
		if(pass == 1)
		printf("%08x NEGF\n",instruction);
		#endif
		makeFloat(opStackDepth-1);
                Inst( "fneg", PPC_FNEG, opStackFloatRegisters[opStackDepth-1], 0, opStackFloatRegisters[opStackDepth-1] );
		opStackLoadInstructionAddr[opStackDepth-1] = 0;
                break;
            case OP_ADDF:
		#if DEBUG_VM
		if(pass == 1)
		printf("%08x ADDF\n",instruction);
		#endif
		makeFloat(opStackDepth-1);
		makeFloat(opStackDepth-2);
                Inst( "fadds", PPC_FADDS, opStackFloatRegisters[opStackDepth-2], opStackFloatRegisters[opStackDepth-2], opStackFloatRegisters[opStackDepth-1] );
		opStackRegType[opStackDepth-1] = 0;
		opStackLoadInstructionAddr[opStackDepth-1] = 0;
		opStackDepth -= 1;
                break;
            case OP_SUBF:
		#if DEBUG_VM
		if(pass == 1)
		printf("%08x SUBF\n",instruction);
		#endif
		makeFloat(opStackDepth-1);
		makeFloat(opStackDepth-2);
                Inst( "fsubs", PPC_FSUBS, opStackFloatRegisters[opStackDepth-2], opStackFloatRegisters[opStackDepth-2], opStackFloatRegisters[opStackDepth-1] );
		opStackRegType[opStackDepth-1] = 0;
		opStackLoadInstructionAddr[opStackDepth-1] = 0;
		opStackDepth -= 1;
                break;
            case OP_DIVF:
		#if DEBUG_VM
		if(pass == 1)
		printf("%08x DIVF\n",instruction);
		#endif
		makeFloat(opStackDepth-1);
		makeFloat(opStackDepth-2);
                Inst( "fdivs", PPC_FDIVS, opStackFloatRegisters[opStackDepth-2], opStackFloatRegisters[opStackDepth-2], opStackFloatRegisters[opStackDepth-1] );
		opStackRegType[opStackDepth-1] = 0;
		opStackLoadInstructionAddr[opStackDepth-1] = 0;
		opStackDepth -= 1;
                break;
            case OP_MULF:
		#if DEBUG_VM
		if(pass == 1)
		printf("%08x MULF\n",instruction);
		#endif
		makeFloat(opStackDepth-1);
		makeFloat(opStackDepth-2);
                Inst4( "fmuls", PPC_FMULS, opStackFloatRegisters[opStackDepth-2], opStackFloatRegisters[opStackDepth-2], 0, opStackFloatRegisters[opStackDepth-1] );
		opStackRegType[opStackDepth-1] = 0;
		opStackLoadInstructionAddr[opStackDepth-1] = 0;
		opStackDepth -= 1;
                break;

            case OP_CVIF:
		#if DEBUG_VM
		if(pass == 1)
		printf("%08x CVIF\n",instruction);
		#endif
		assertInteger(opStackDepth-1);
		//makeInteger(opStackDepth-1);
                v = (int)&itofConvert;
                InstImmU( "addis", PPC_ADDIS, R_EA, 0, (v >> 16)&0xffff );
                InstImmU( "ori", PPC_ORI, R_EA, R_EA, v & 0xffff );
                InstImmU( "xoris", PPC_XORIS, opStackIntRegisters[opStackDepth-1], opStackIntRegisters[opStackDepth-1], 0x8000 );
                InstImm( "stw", PPC_STW, opStackIntRegisters[opStackDepth-1], R_EA, 12 );
                InstImm( "lfd", PPC_LFD, opStackFloatRegisters[opStackDepth-1], R_EA, 0 );
		Inst( "ori", PPC_ORI, 0, 0, 0);
		Inst( "ori", PPC_ORI, 0, 0, 0);
		Inst( "ori", PPC_ORI, 0, 0, 0);
                InstImm( "lfd", PPC_LFD, 13, R_EA, 8 );
                Inst( "fsub", PPC_FSUB, opStackFloatRegisters[opStackDepth-1], 13, opStackFloatRegisters[opStackDepth-1] );
		opStackRegType[opStackDepth-1] = 2;
		opStackLoadInstructionAddr[opStackDepth-1] = 0;
    //            Inst( PPC_FRSP, R_TOP, 0, R_TOP );
                break;
            case OP_CVFI:
		#if DEBUG_VM
		if(pass == 1)
		printf("%08x CVFI\n",instruction);
		#endif
		makeFloat(opStackDepth-1);

		InstImm( "addi", PPC_ADDI, R_OPSTACK, R_OPSTACK, opStackDepth*4);

                Inst( "fctiwz", PPC_FCTIWZ, opStackFloatRegisters[opStackDepth-1], 0, opStackFloatRegisters[opStackDepth-1] );
                Inst( "stfiwx", PPC_STFIWX, opStackFloatRegisters[opStackDepth-1], 0, R_OPSTACK );		// save value to opstack (dummy area now)
		Inst( "ori", PPC_ORI, 0, 0, 0);
		Inst( "ori", PPC_ORI, 0, 0, 0);
		Inst( "ori", PPC_ORI, 0, 0, 0);
		Inst( "ori", PPC_ORI, 0, 0, 0);
                InstImm( "lwz", PPC_LWZ, opStackIntRegisters[opStackDepth-1], R_OPSTACK, 0 );
		
		InstImm( "addi", PPC_ADDI, R_OPSTACK, R_OPSTACK, -opStackDepth*4);
		
		opStackRegType[opStackDepth-1] = 1;
		opStackLoadInstructionAddr[opStackDepth-1] = 0;
                break;
            case OP_SEX8:
		#if DEBUG_VM
		if(pass == 1)
		printf("%08x SEX8\n",instruction);
		#endif
		assertInteger(opStackDepth-1);
                Inst( "extsb", PPC_EXTSB, opStackIntRegisters[opStackDepth-1], opStackIntRegisters[opStackDepth-1], 0 );
		opStackLoadInstructionAddr[opStackDepth-1] = 0;
                break;
            case OP_SEX16:
		#if DEBUG_VM
		if(pass == 1)
		printf("%08x SEX16\n",instruction);
		#endif
		assertInteger(opStackDepth-1);
                Inst( "extsh", PPC_EXTSH, opStackIntRegisters[opStackDepth-1], opStackIntRegisters[opStackDepth-1], 0 );
		opStackLoadInstructionAddr[opStackDepth-1] = 0;
                break;

            case OP_BLOCK_COPY:
                v = Constant4() >> 2;
		#if DEBUG_VM
		if(pass == 1)
		printf("%08x BLOCK_COPY\t%08lx\n",instruction,v<<2);
		#endif
		assert(opStackDepth >= 2);
		assertInteger(opStackDepth-1);
		assertInteger(opStackDepth-2);
                InstImmU( "addi", PPC_ADDI, R_EA, 0, v );				// count
				// FIXME: range check
              	Inst( "mtctr", PPC_MTSPR, R_EA, 9, 0 );					// move to count register

                Inst( "add", PPC_ADD, opStackIntRegisters[opStackDepth-1], opStackIntRegisters[opStackDepth-1], R_MEMBASE );
                InstImm( "addi", PPC_ADDI, opStackIntRegisters[opStackDepth-1], opStackIntRegisters[opStackDepth-1], -4 );
                Inst( "add", PPC_ADD, opStackIntRegisters[opStackDepth-2], opStackIntRegisters[opStackDepth-2], R_MEMBASE );
                InstImm( "addi", PPC_ADDI, opStackIntRegisters[opStackDepth-2], opStackIntRegisters[opStackDepth-2], -4 );

                InstImm( "lwzu", PPC_LWZU, R_EA, opStackIntRegisters[opStackDepth-1], 4 );		// source
                InstImm( "stwu", PPC_STWU, R_EA, opStackIntRegisters[opStackDepth-2], 4 );	// dest
                Inst( "b", PPC_BC | 0xfff8 , 16, 0, 0 );					// loop
		opStackRegType[opStackDepth-1] = 0;
		opStackRegType[opStackDepth-2] = 0;
		opStackLoadInstructionAddr[opStackDepth-1] = 0;
		opStackLoadInstructionAddr[opStackDepth-2] = 0;
		opStackDepth -= 2;
                break;

            case OP_JUMP:
		#if DEBUG_VM
		if(pass == 1)
		printf("%08x JUMP\n",instruction);
		#endif
		assert(opStackDepth == 1);
		assertInteger(opStackDepth-1);

                Inst( "rlwinm", PPC_RLWINM | ( 29 << 1 ), opStackIntRegisters[opStackDepth-1], opStackIntRegisters[opStackDepth-1], 2 );
		// FIXME: range check
		Inst( "lwzx", PPC_LWZX, opStackIntRegisters[opStackDepth-1], opStackIntRegisters[opStackDepth-1], R_INSTRUCTIONS );
                Inst( "mtctr", PPC_MTSPR, opStackIntRegisters[opStackDepth-1], 9, 0 );		// move to count register
                Inst( "bctr", PPC_BCCTR, 20, 0, 0 );		// jump to the count register
		opStackRegType[opStackDepth-1] = 0;
		opStackLoadInstructionAddr[opStackDepth-1] = 0;
		opStackDepth -= 1;
                break;
            default:
                Com_Error( ERR_DROP, "VM_CompilePPC: bad opcode %i at instruction %i, offset %i", op, instruction, pc );
            }
	    pop0 = pop1;
	    pop1 = op;
		assert(opStackDepth >= 0);
		assert(opStackDepth < OP_STACK_MAX_DEPTH);
		
		//printf("%4d\t%s\n",opStackDepth,opnames[op]);
        }

	Com_Printf( "VM file %s pass %d compiled to %i bytes of code\n", vm->name, (pass+1), compiledOfs*4 );

    	if ( pass == 0 ) {
	    // copy to an exact size buffer on the hunk
	    vm->codeLength = compiledOfs * 4;
	    vm->codeBase = Hunk_Alloc( vm->codeLength, h_low );
	    Com_Memcpy( vm->codeBase, buf, vm->codeLength );
	    
	    //printf("codeBase: %p\n",vm->codeBase);
	    
	    Z_Free( buf );
	
	    // offset all the instruction pointers for the new location
	    for ( i = 0 ; i < header->instructionCount ; i++ ) {
		vm->instructionPointers[i] += (int)vm->codeBase;
		//printf("%08x %08lx\n",i,vm->instructionPointers[i]);
	    }

	    // go back over it in place now to fixup reletive jump targets
	    buf = (unsigned *)vm->codeBase;
	}
    }
    if(0)
    {
	char buf[256];
	printf("wait..\n");
	gets(buf);
    }
    Z_Free( jused );
}

/*
==============
VM_CallCompiled

This function is called directly by the generated code
==============
*/
int	VM_CallCompiled( vm_t *vm, int *args ) {
	int		stack[1024];
	int		programStack;
	int		stackOnEntry;
	byte	*image;

	currentVM = vm;

	//printf("VM_CallCompiled: %p   %08lx %08lx %08lx\n",
	//	vm, args[0],args[1],args[2]);
		
	// interpret the code
	vm->currentlyInterpreting = qtrue;

	// we might be called recursively, so this might not be the very top
	programStack = vm->programStack;
	stackOnEntry = programStack;
	image = vm->dataBase;
	
	// set up the stack frame 
	programStack -= 48;

	*(int *)&image[ programStack + 44] = args[9];
	*(int *)&image[ programStack + 40] = args[8];
	*(int *)&image[ programStack + 36] = args[7];
	*(int *)&image[ programStack + 32] = args[6];
	*(int *)&image[ programStack + 28] = args[5];
	*(int *)&image[ programStack + 24] = args[4];
	*(int *)&image[ programStack + 20] = args[3];
	*(int *)&image[ programStack + 16] = args[2];
	*(int *)&image[ programStack + 12] = args[1];
	*(int *)&image[ programStack + 8 ] = args[0];
	*(int *)&image[ programStack + 4 ] = 0;	// return stack
	*(int *)&image[ programStack ] = -1;	// will terminate the loop on return

	// Cheesy... manually save registers used by VM call...
	// off we go into generated code...
	// the PPC calling standard says the parms will all go into R3 - R11, so 
	// no special asm code is needed here
#ifdef __GNUC__
	((void(*)(int, int, int, int, int, int, int, int))(vm->codeBase))( 
		programStack, (int)&stack, 
		(int)image, vm->dataMask, (int)&AsmCall, 
		(int)vm->instructionPointers, vm->instructionPointersLength,
        (int)vm );
#else
	((void(*)(int, int, int, int, int, int, int, int))(vm->codeBase))( 
		programStack, (int)&stack, 
		(int)image, vm->dataMask, *(int *)&AsmCall /* skip function pointer header */, 
		(int)vm->instructionPointers, vm->instructionPointersLength,
        (int)vm );
#endif
	vm->programStack = stackOnEntry;

    vm->currentlyInterpreting = qfalse;

	return stack[1];
}


/*
==================
AsmCall

Put this at end of file because gcc messes up debug line numbers 
==================
*/
#ifdef __GNUC__

void AsmCall( void ) {
asm (
     // pop off the destination instruction
"    lwz		r12,0(r4)	\n"	// RG_TOP, 0(RG_OPSTACK)
"    addi	r4,r4,-4		\n"	// RG_OPSTACK, RG_OPSTACK, -4 \n"

    // see if it is a system trap
"    cmpwi	r12,0			\n"	// RG_TOP, 0 \n"
"    bc		12,0, systemTrap	\n"

    // calling another VM function, so lookup in instructionPointers
"    slwi	r12,r12,2		\n"	// RG_TOP,RG_TOP,2
                        // FIXME: range check
"    lwzx	r12, r8, r12		\n"	// RG_TOP, RG_INSTRUCTIONS(RG_TOP)	
"    mtctr	r12			\n"	// RG_TOP
);

#if defined(MACOS_X) && defined(__OPTIMIZE__)
    // On Mac OS X, gcc doesn't push a frame when we are optimized, so trying to tear it down results in grave disorder.
#warning Mac OS X optimization on, not popping GCC AsmCall frame
#else
    // Mac OS X Server and unoptimized compiles include a GCC AsmCall frame
    asm (
"	lwz		r1,0(r1)	\n"	// pop off the GCC AsmCall frame
"	lmw		r30,-8(r1)	\n"
);
#endif

asm (
"	    bcctr	20,0		\n" // when it hits a leave, it will branch to the current link register

    // calling a system trap
"systemTrap:				\n"
	// convert to positive system call number
"	subfic	r12,r12,-1		\n"

    // save all our registers, including the current link register
"    mflr	r13			\n"	// RG_SECOND		// copy off our link register
"	addi	r1,r1,-92		\n"	// required 24 byets of linkage, 32 bytes of parameter, plus our saves
"    stw		r3,56(r1)	\n"	// RG_STACK, -36(REAL_STACK)
"    stw		r4,60(r1)	\n"	// RG_OPSTACK, 4(RG_REAL_STACK)
"    stw		r5,64(r1)	\n"	// RG_MEMBASE, 8(RG_REAL_STACK)
"    stw		r6,68(r1)	\n"	// RG_MEMMASK, 12(RG_REAL_STACK)
"    stw		r7,72(r1)	\n"	// RG_ASMCALL, 16(RG_REAL_STACK)
"    stw		r8,76(r1)	\n"	// RG_INSTRUCTIONS, 20(RG_REAL_STACK)
"    stw		r9,80(r1)	\n"	// RG_NUM_INSTRUCTIONS, 24(RG_REAL_STACK)
"    stw		r10,84(r1)	\n"	// RG_VM, 28(RG_REAL_STACK)
"    stw		r13,88(r1)	\n"	// RG_SECOND, 32(RG_REAL_STACK)	// link register

    // save the vm stack position to allow recursive VM entry
"    addi	r13,r3,-4		\n"	// RG_TOP, RG_STACK, -4
"    stw		r13,0(r10)	\n"	//RG_TOP, VM_OFFSET_PROGRAM_STACK(RG_VM)

    // save the system call number as the 0th parameter
"    add		r3,r3,r5	\n"	// r3,  RG_STACK, RG_MEMBASE		// r3 is the first parameter to vm->systemCalls
"    stwu	r12,4(r3)		\n"	// RG_TOP, 4(r3)

    // make the system call with the address of all the VM parms as a parameter
    // vm->systemCalls( &parms )
"    lwz		r12,4(r10)	\n"	// RG_TOP, VM_OFFSET_SYSTEM_CALL(RG_VM)
"    mtctr	r12			\n"	// RG_TOP
"    bcctrl	20,0			\n"
"    mr		r12,r3			\n"	// RG_TOP, r3

    // pop our saved registers
"   	lwz		r3,56(r1)	\n"	// RG_STACK, 0(RG_REAL_STACK)
"   	lwz		r4,60(r1)	\n"	// RG_OPSTACK, 4(RG_REAL_STACK)
"   	lwz		r5,64(r1)	\n"	// RG_MEMBASE, 8(RG_REAL_STACK)
"   	lwz		r6,68(r1)	\n"	// RG_MEMMASK, 12(RG_REAL_STACK)
"   	lwz		r7,72(r1)	\n"	// RG_ASMCALL, 16(RG_REAL_STACK)
"   	lwz		r8,76(r1)	\n"	// RG_INSTRUCTIONS, 20(RG_REAL_STACK)
"   	lwz		r9,80(r1)	\n"	// RG_NUM_INSTRUCTIONS, 24(RG_REAL_STACK)
"   	lwz		r10,84(r1)	\n"	// RG_VM, 28(RG_REAL_STACK)
"   	lwz		r13,88(r1)	\n"	// RG_SECOND, 32(RG_REAL_STACK)
"    addi	r1,r1,92		\n"	// RG_REAL_STACK, RG_REAL_STACK, 36

    // restore the old link register
"    mtlr	r13			\n"	// RG_SECOND

    // save off the return value
"    stwu	r12,4(r4)		\n"	// RG_TOP, 0(RG_OPSTACK)

	// GCC adds its own prolog / epliog code
 );
}
#else

// codewarrior version

void asm AsmCall( void ) {

    // pop off the destination instruction

    lwz		r12,0(r4)	// RG_TOP, 0(RG_OPSTACK)

    addi	r4,r4,-4	// RG_OPSTACK, RG_OPSTACK, -4



    // see if it is a system trap

    cmpwi	r12,0		// RG_TOP, 0

    bc		12,0, systemTrap



    // calling another VM function, so lookup in instructionPointers

    slwi	r12,r12,2		// RG_TOP,RG_TOP,2

                        // FIXME: range check

    lwzx	r12, r8, r12	// RG_TOP, RG_INSTRUCTIONS(RG_TOP)	

    mtctr	r12			// RG_TOP



    bcctr	20,0		// when it hits a leave, it will branch to the current link register



    // calling a system trap

systemTrap:

	// convert to positive system call number

	subfic	r12,r12,-1



    // save all our registers, including the current link register

    mflr	r13			// RG_SECOND		// copy off our link register

	addi	r1,r1,-92	// required 24 byets of linkage, 32 bytes of parameter, plus our saves

    stw		r3,56(r1)	// RG_STACK, -36(REAL_STACK)

    stw		r4,60(r1)	// RG_OPSTACK, 4(RG_REAL_STACK)

    stw		r5,64(r1)	// RG_MEMBASE, 8(RG_REAL_STACK)

    stw		r6,68(r1)	// RG_MEMMASK, 12(RG_REAL_STACK)

    stw		r7,72(r1)	// RG_ASMCALL, 16(RG_REAL_STACK)

    stw		r8,76(r1)	// RG_INSTRUCTIONS, 20(RG_REAL_STACK)

    stw		r9,80(r1)	// RG_NUM_INSTRUCTIONS, 24(RG_REAL_STACK)

    stw		r10,84(r1)	// RG_VM, 28(RG_REAL_STACK)

    stw		r13,88(r1)	// RG_SECOND, 32(RG_REAL_STACK)	// link register



    // save the vm stack position to allow recursive VM entry

    addi	r13,r3,-4	// RG_TOP, RG_STACK, -4

    stw		r13,0(r10)	//RG_TOP, VM_OFFSET_PROGRAM_STACK(RG_VM)



    // save the system call number as the 0th parameter

    add		r3,r3,r5	// r3,  RG_STACK, RG_MEMBASE		// r3 is the first parameter to vm->systemCalls

    stwu	r12,4(r3)	// RG_TOP, 4(r3)



    // make the system call with the address of all the VM parms as a parameter

    // vm->systemCalls( &parms )

    lwz		r12,4(r10)	// RG_TOP, VM_OFFSET_SYSTEM_CALL(RG_VM)

    

    // perform macos cross fragment fixup crap

    lwz		r9,0(r12)

    stw		r2,52(r1)	// save old TOC

	lwz		r2,4(r12)

	    

    mtctr	r9			// RG_TOP

    bcctrl	20,0

    

    lwz		r2,52(r1)	// restore TOC

    

    mr		r12,r3		// RG_TOP, r3



    // pop our saved registers

   	lwz		r3,56(r1)	// RG_STACK, 0(RG_REAL_STACK)

   	lwz		r4,60(r1)	// RG_OPSTACK, 4(RG_REAL_STACK)

   	lwz		r5,64(r1)	// RG_MEMBASE, 8(RG_REAL_STACK)

   	lwz		r6,68(r1)	// RG_MEMMASK, 12(RG_REAL_STACK)

   	lwz		r7,72(r1)	// RG_ASMCALL, 16(RG_REAL_STACK)

   	lwz		r8,76(r1)	// RG_INSTRUCTIONS, 20(RG_REAL_STACK)

   	lwz		r9,80(r1)	// RG_NUM_INSTRUCTIONS, 24(RG_REAL_STACK)

   	lwz		r10,84(r1)	// RG_VM, 28(RG_REAL_STACK)

   	lwz		r13,88(r1)	// RG_SECOND, 32(RG_REAL_STACK)

    addi	r1,r1,92	// RG_REAL_STACK, RG_REAL_STACK, 36



    // restore the old link register

    mtlr	r13			// RG_SECOND



    // save off the return value

    stwu	r12,4(r4)	// RG_TOP, 0(RG_OPSTACK)



	blr

}




#endif

/*
===========================================================================

Return to Castle Wolfenstein single player GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the Return to Castle Wolfenstein single player GPL Source Code (RTCW SP Source Code).  

RTCW SP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW SP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW SP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW SP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW SP Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

// vm_x86.c -- load time compiler and execution environment for x86

#include "vm_local.h"

#ifndef _WIN32
#include <sys/mman.h> // for PROT_ stuff
#endif

/*

  eax	scratch
  ebx	scratch
  ecx	scratch (required for shifts)
  edx	scratch (required for divisions)
  esi	program stack
  edi	opstack

*/

static byte    *buf;
static int compiledOfs;
static byte    *code;
static int pc;

static int     *instructionPointers;

#ifdef _WIN32
void AsmCall( void );
int _ftol( float );

//static	int		ftolPtr = (int)_ftol;
static int asmCallPtr = (int)AsmCall;

#else

void doAsmCall( void );

static int asmCallPtr = (int)doAsmCall;
#endif


/*
=================
AsmCall
=================
*/
#ifdef _WIN32
__declspec( naked ) void AsmCall( void ) {
	static int programStack;
	static int     *opStack;
	static int syscallNum;

	__asm {
		mov eax, dword ptr [edi]
		sub edi, 4
		or      eax,eax
		jl systemCall
		// calling another vm function
		shl eax,2
		add eax, dword ptr [instructionPointers]
		call dword ptr [eax]
		ret
systemCall:

		// convert negative num to system call number
		// and store right before the first arg
		neg eax
		dec eax

		mov dword ptr syscallNum, eax   // so C code can get at it
		mov dword ptr programStack, esi // so C code can get at it
		mov dword ptr opStack, edi

		push ecx
		push esi                        // we may call recursively, so the
		push edi                        // statics aren't guaranteed to be around
	}

	// save the stack to allow recursive VM entry
	currentVM->programStack = programStack - 4;
	*( int * )( (byte *)currentVM->dataBase + programStack + 4 ) = syscallNum;
//VM_LogSyscalls(  (int *)((byte *)currentVM->dataBase + programStack + 4) );
	*( opStack + 1 ) = currentVM->systemCall( ( int * )( (byte *)currentVM->dataBase + programStack + 4 ) );

	_asm {
		pop edi
		pop esi
		pop ecx
		add edi, 4      // we added the return value

		ret
	}

}

#else

static int callProgramStack;
static int     *callOpStack;
static int callSyscallNum;

void callAsmCall( void ) {
	// save the stack to allow recursive VM entry
	currentVM->programStack = callProgramStack - 4;
	*( int * )( (byte *)currentVM->dataBase + callProgramStack + 4 ) = callSyscallNum;
//VM_LogSyscalls(  (int *)((byte *)currentVM->dataBase + programStack + 4) );
	*( callOpStack + 1 ) = currentVM->systemCall( ( int * )( (byte *)currentVM->dataBase + callProgramStack + 4 ) );
}

void AsmCall( void ) {
	__asm__( "doAsmCall:                \n\t"\
			 "	movl (%%edi),%%eax			\n\t"\
			 "	subl $4,%%edi				\n\t"\
			 "   orl %%eax,%%eax				\n\t"\
			 "	jl systemCall				\n\t"\
			 "	shll $2,%%eax				\n\t"\
			 "	addl %3,%%eax				\n\t"\
			 "	call *(%%eax)				\n\t"\
			 "	jmp doret					\n\t"\
			 "systemCall:					\n\t"\
			 "	negl %%eax					\n\t"\
			 "	decl %%eax					\n\t"\
			 "	movl %%eax,%0				\n\t"\
			 "	movl %%esi,%1				\n\t"\
			 "	movl %%edi,%2				\n\t"\
			 "	pushl %%ecx					\n\t"\
			 "	pushl %%esi					\n\t"\
			 "	pushl %%edi					\n\t"\
			 "	call callAsmCall			\n\t"\
			 "	popl %%edi					\n\t"\
			 "	popl %%esi					\n\t"\
			 "	popl %%ecx					\n\t"\
			 "	addl $4,%%edi				\n\t"\
			 "doret:							\n\t"\
			 "	ret							\n\t"\
			 : "=rm" ( callSyscallNum ), "=rm" ( callProgramStack ), "=rm" ( callOpStack ) \
			 : "rm" ( instructionPointers )	\
			 : "ax", "di", "si", "cx" \
			 );
}
#endif


static int  Constant4( void ) {
	int v;

	v = code[pc] | ( code[pc + 1] << 8 ) | ( code[pc + 2] << 16 ) | ( code[pc + 3] << 24 );
	pc += 4;
	return v;
}

static int  Constant1( void ) {
	int v;

	v = code[pc];
	pc += 1;
	return v;
}

static void Emit1( int v ) {
	buf[ compiledOfs ] = v;
	compiledOfs++;
}
#if 0
static void Emit2( int v ) {
	Emit1( v & 255 );
	Emit1( ( v >> 8 ) & 255 );
}
#endif
static void Emit4( int v ) {
	Emit1( v & 255 );
	Emit1( ( v >> 8 ) & 255 );
	Emit1( ( v >> 16 ) & 255 );
	Emit1( ( v >> 24 ) & 255 );
}

static int Hex( int c ) {
	if ( c >= 'a' && c <= 'f' ) {
		return 10 + c - 'a';
	}
	if ( c >= 'A' && c <= 'F' ) {
		return 10 + c - 'A';
	}
	if ( c >= '0' && c <= '9' ) {
		return c - '0';
	}

	Com_Error( ERR_DROP, "Hex: bad char '%c'", c );

	return 0;
}
static void EmitString( const char *string ) {
	int c1, c2;
	int v;

	while ( 1 ) {
		c1 = string[0];
		c2 = string[1];

		v = ( Hex( c1 ) << 4 ) | Hex( c2 );
		Emit1( v );

		if ( !string[2] ) {
			break;
		}
		string += 3;
	}
}

/*
=================
VM_Compile
=================
*/
void VM_Compile( vm_t *vm, vmHeader_t *header ) {
	int op;
	int maxLength;
	int v;
	int i;
	int instruction;
	int lastConst;

	// allocate a very large temp buffer, we will shrink it later
	maxLength = header->codeLength * 8;
	buf = Z_Malloc( maxLength );

	// translate all instructions
	pc = 0;
	instruction = 0;
	code = (byte *)header + header->codeOffset;
	compiledOfs = 0;

	while ( instruction < header->instructionCount ) {
		if ( compiledOfs > maxLength - 16 ) {
			Com_Error( ERR_FATAL, "VM_CompileX86: maxLength exceeded" );
		}

		vm->instructionPointers[ instruction ] = compiledOfs;
		instruction++;

		op = code[ pc ];
		if ( pc > header->codeLength ) {
			Com_Error( ERR_FATAL, "VM_CompileX86: pc > header->codeLength" );
		}

		pc++;
		switch ( op ) {
		case 0:
			break;
		case OP_BREAK:
			EmitString( "CC" );          // int 3
			break;
		case OP_ENTER:
			EmitString( "81 EE" );       // sub	esi, 0x12345678
			Emit4( Constant4() );
			break;
		case OP_CONST:
			EmitString( "83 C7 04" );    //	add edi,4
			EmitString( "C7 07" );       // mov	dword ptr [edi], 0x12345678
			lastConst = Constant4();
			Emit4( lastConst );
			break;
		case OP_LOCAL:
			EmitString( "83 C7 04" );    //	add edi,4
			EmitString( "8D 86" );       // lea eax, [0x12345678 + esi]
			Emit4( Constant4() );
			EmitString( "89 07" );       // mov	dword ptr [edi], eax
			break;
		case OP_ARG:
			EmitString( "8B 07" );       // mov	eax,dword ptr [edi]
			EmitString( "89 86" );       // mov	dword ptr [esi+database],eax
			// FIXME: range check
			Emit4( Constant1() + (int)vm->dataBase );
			EmitString( "83 EF 04" );    // sub	edi,4
			break;
		case OP_CALL:
			EmitString( "C7 86" );       // mov dword ptr [esi+database],0x12345678
			Emit4( (int)vm->dataBase );
			Emit4( pc );
			EmitString( "FF 15" );       // call asmCallPtr
			Emit4( (int)&asmCallPtr );
			break;
		case OP_PUSH:
			EmitString( "83 C7 04" );    // add edi,4
			break;
		case OP_POP:
			EmitString( "83 EF 04" );    // sub edi, 4
			break;
		case OP_LEAVE:
			v = Constant4();
			EmitString( "81 C6" );       // add	esi, 0x12345678
			Emit4( v );
			EmitString( "C3" );          // ret
			break;
		case OP_LOAD4:
			EmitString( "8B 1F" );       // mov	ebx, dword ptr [edi]
			EmitString( "81 E3" );       // and ebx, 0x12345678
			Emit4( vm->dataMask );
			EmitString( "8B 83" );       // mov	eax, dword ptr [ebx + 0x12345678]
			Emit4( (int)vm->dataBase );
			EmitString( "89 07" );       // mov dword ptr [edi], eax
			break;
		case OP_LOAD2:
			EmitString( "8B 1F" );       // mov	ebx, dword ptr [edi]
			EmitString( "81 E3" );       // and ebx, 0x12345678
			Emit4( vm->dataMask );
			EmitString( "0F B7 83" );    // movzx	eax, word ptr [ebx + 0x12345678]
			Emit4( (int)vm->dataBase );
			EmitString( "89 07" );       // mov dword ptr [edi], eax
			break;
		case OP_LOAD1:
			EmitString( "8B 1F" );       // mov	ebx, dword ptr [edi]
			EmitString( "81 E3" );       // and ebx, 0x12345678
			Emit4( vm->dataMask );
			EmitString( "0F B6 83" );    // movzx eax, byte ptr [ebx + 0x12345678]
			Emit4( (int)vm->dataBase );
			EmitString( "89 07" );       // mov dword ptr [edi], eax
			break;
		case OP_STORE4:
			EmitString( "8B 5F FC" );    // mov	ebx, dword ptr [edi-4]
			EmitString( "81 E3" );       // and ebx, 0x12345678
			Emit4( vm->dataMask & ~3 );
			EmitString( "8B 07" );       // mov eax, dword ptr [edi];
			EmitString( "89 83" );       // mov dword ptr [ebx+0x12345678], eax
			Emit4( (int)vm->dataBase );
			EmitString( "83 EF 08" );    // sub edi, 8
			break;
		case OP_STORE2:
			EmitString( "8B 5F FC" );    // mov	ebx, dword ptr [edi-4]
			EmitString( "81 E3" );       // and ebx, 0x12345678
			Emit4( vm->dataMask & ~1 );
			EmitString( "8B 07" );       // mov eax, dword ptr [edi];
			EmitString( "66 89 83" );    // mov word ptr [ebx+0x12345678], eax
			Emit4( (int)vm->dataBase );
			EmitString( "83 EF 08" );    // sub edi, 8
			break;
		case OP_STORE1:
			EmitString( "8B 5F FC" );    // mov	ebx, dword ptr [edi-4]
			EmitString( "81 E3" );       // and ebx, 0x12345678
			Emit4( vm->dataMask );
			EmitString( "8B 07" );       // mov eax, dword ptr [edi];
			EmitString( "88 83" );       // mov byte ptr [ebx+0x12345678], eax
			Emit4( (int)vm->dataBase );
			EmitString( "83 EF 08" );    // sub edi, 8
			break;

		case OP_EQ:
			EmitString( "83 EF 08" );    // sub	edi,8
			EmitString( "8B 47 04" );    // mov	eax, dword ptr [edi+4]
			EmitString( "3B 47 08" );    // cmp	eax, dword ptr [edi+8]
			EmitString( "75 06" );       // jne +6
			EmitString( "FF 25" );       // jmp	[0x12345678]
			Emit4( (int)vm->instructionPointers + Constant4() * 4 );
			break;
		case OP_NE:
			EmitString( "83 EF 08" );    // sub	edi,8
			EmitString( "8B 47 04" );    // mov	eax, dword ptr [edi+4]
			EmitString( "3B 47 08" );    // cmp	eax, dword ptr [edi+8]
			EmitString( "74 06" );       // je +6
			EmitString( "FF 25" );       // jmp	[0x12345678]
			Emit4( (int)vm->instructionPointers + Constant4() * 4 );
			break;
		case OP_LTI:
			EmitString( "83 EF 08" );    // sub	edi,8
			EmitString( "8B 47 04" );    // mov	eax, dword ptr [edi+4]
			EmitString( "3B 47 08" );    // cmp	eax, dword ptr [edi+8]
			EmitString( "7D 06" );       // jnl +6
			EmitString( "FF 25" );       // jmp	[0x12345678]
			Emit4( (int)vm->instructionPointers + Constant4() * 4 );
			break;
		case OP_LEI:
			EmitString( "83 EF 08" );    // sub	edi,8
			EmitString( "8B 47 04" );    // mov	eax, dword ptr [edi+4]
			EmitString( "3B 47 08" );    // cmp	eax, dword ptr [edi+8]
			EmitString( "7F 06" );       // jnle +6
			EmitString( "FF 25" );       // jmp	[0x12345678]
			Emit4( (int)vm->instructionPointers + Constant4() * 4 );
			break;
		case OP_GTI:
			EmitString( "83 EF 08" );    // sub	edi,8
			EmitString( "8B 47 04" );    // mov	eax, dword ptr [edi+4]
			EmitString( "3B 47 08" );    // cmp	eax, dword ptr [edi+8]
			EmitString( "7E 06" );       // jng +6
			EmitString( "FF 25" );       // jmp	[0x12345678]
			Emit4( (int)vm->instructionPointers + Constant4() * 4 );
			break;
		case OP_GEI:
			EmitString( "83 EF 08" );    // sub	edi,8
			EmitString( "8B 47 04" );    // mov	eax, dword ptr [edi+4]
			EmitString( "3B 47 08" );    // cmp	eax, dword ptr [edi+8]
			EmitString( "7C 06" );       // jnge +6
			EmitString( "FF 25" );       // jmp	[0x12345678]
			Emit4( (int)vm->instructionPointers + Constant4() * 4 );
			break;
		case OP_LTU:
			EmitString( "83 EF 08" );    // sub	edi,8
			EmitString( "8B 47 04" );    // mov	eax, dword ptr [edi+4]
			EmitString( "3B 47 08" );    // cmp	eax, dword ptr [edi+8]
			EmitString( "73 06" );       // jnb +6
			EmitString( "FF 25" );       // jmp	[0x12345678]
			Emit4( (int)vm->instructionPointers + Constant4() * 4 );
			break;
		case OP_LEU:
			EmitString( "83 EF 08" );    // sub	edi,8
			EmitString( "8B 47 04" );    // mov	eax, dword ptr [edi+4]
			EmitString( "3B 47 08" );    // cmp	eax, dword ptr [edi+8]
			EmitString( "77 06" );       // jnbe +6
			EmitString( "FF 25" );       // jmp	[0x12345678]
			Emit4( (int)vm->instructionPointers + Constant4() * 4 );
			break;
		case OP_GTU:
			EmitString( "83 EF 08" );    // sub	edi,8
			EmitString( "8B 47 04" );    // mov	eax, dword ptr [edi+4]
			EmitString( "3B 47 08" );    // cmp	eax, dword ptr [edi+8]
			EmitString( "76 06" );       // jna +6
			EmitString( "FF 25" );       // jmp	[0x12345678]
			Emit4( (int)vm->instructionPointers + Constant4() * 4 );
			break;
		case OP_GEU:
			EmitString( "83 EF 08" );    // sub	edi,8
			EmitString( "8B 47 04" );    // mov	eax, dword ptr [edi+4]
			EmitString( "3B 47 08" );    // cmp	eax, dword ptr [edi+8]
			EmitString( "72 06" );       // jnae +6
			EmitString( "FF 25" );       // jmp	[0x12345678]
			Emit4( (int)vm->instructionPointers + Constant4() * 4 );
			break;
		case OP_EQF:
			EmitString( "83 EF 08" );    // sub	edi,8
			EmitString( "D9 47 04" );    // fld dword ptr [edi+4]
			EmitString( "D8 5F 08" );    // fcomp dword ptr [edi+8]
			EmitString( "DF E0" );       // fnstsw ax
			EmitString( "F6 C4 40" );    // test	ah,0x40
			EmitString( "74 06" );       // je +6
			EmitString( "FF 25" );       // jmp	[0x12345678]
			Emit4( (int)vm->instructionPointers + Constant4() * 4 );
			break;
		case OP_NEF:
			EmitString( "83 EF 08" );    // sub	edi,8
			EmitString( "D9 47 04" );    // fld dword ptr [edi+4]
			EmitString( "D8 5F 08" );    // fcomp dword ptr [edi+8]
			EmitString( "DF E0" );       // fnstsw ax
			EmitString( "F6 C4 40" );    // test	ah,0x40
			EmitString( "75 06" );       // jne +6
			EmitString( "FF 25" );       // jmp	[0x12345678]
			Emit4( (int)vm->instructionPointers + Constant4() * 4 );
			break;
		case OP_LTF:
			EmitString( "83 EF 08" );    // sub	edi,8
			EmitString( "D9 47 04" );    // fld dword ptr [edi+4]
			EmitString( "D8 5F 08" );    // fcomp dword ptr [edi+8]
			EmitString( "DF E0" );       // fnstsw ax
			EmitString( "F6 C4 01" );    // test	ah,0x01
			EmitString( "74 06" );       // je +6
			EmitString( "FF 25" );       // jmp	[0x12345678]
			Emit4( (int)vm->instructionPointers + Constant4() * 4 );
			break;
		case OP_LEF:
			EmitString( "83 EF 08" );    // sub	edi,8
			EmitString( "D9 47 04" );    // fld dword ptr [edi+4]
			EmitString( "D8 5F 08" );    // fcomp dword ptr [edi+8]
			EmitString( "DF E0" );       // fnstsw ax
			EmitString( "F6 C4 41" );    // test	ah,0x41
			EmitString( "74 06" );       // je +6
			EmitString( "FF 25" );       // jmp	[0x12345678]
			Emit4( (int)vm->instructionPointers + Constant4() * 4 );
			break;
		case OP_GTF:
			EmitString( "83 EF 08" );    // sub	edi,8
			EmitString( "D9 47 04" );    // fld dword ptr [edi+4]
			EmitString( "D8 5F 08" );    // fcomp dword ptr [edi+8]
			EmitString( "DF E0" );       // fnstsw ax
			EmitString( "F6 C4 41" );    // test	ah,0x41
			EmitString( "75 06" );       // jne +6
			EmitString( "FF 25" );       // jmp	[0x12345678]
			Emit4( (int)vm->instructionPointers + Constant4() * 4 );
			break;
		case OP_GEF:
			EmitString( "83 EF 08" );    // sub	edi,8
			EmitString( "D9 47 04" );    // fld dword ptr [edi+4]
			EmitString( "D8 5F 08" );    // fcomp dword ptr [edi+8]
			EmitString( "DF E0" );       // fnstsw ax
			EmitString( "F6 C4 01" );    // test	ah,0x01
			EmitString( "75 06" );       // jne +6
			EmitString( "FF 25" );       // jmp	[0x12345678]
			Emit4( (int)vm->instructionPointers + Constant4() * 4 );
			break;
		case OP_NEGI:
			EmitString( "F7 1F" );       // neg dword ptr [edi]
			break;
		case OP_ADD:
			EmitString( "8B 07" );       // mov eax, dword ptr [edi]
			EmitString( "01 47 FC" );    // add dword ptr [edi-4],eax
			EmitString( "83 EF 04" );    // sub edi,4
			break;
		case OP_SUB:
			EmitString( "8B 07" );       // mov eax, dword ptr [edi]
			EmitString( "29 47 FC" );    // sub dword ptr [edi-4],eax
			EmitString( "83 EF 04" );    // sub edi,4
			break;
		case OP_DIVI:
			EmitString( "8B 47 FC" );    // mov eax,dword ptr [edi-4]
			EmitString( "99" );          // cdq
			EmitString( "F7 3F" );       // idiv dword ptr [edi]
			EmitString( "89 47 FC" );    // mov dword ptr [edi-4],eax
			EmitString( "83 EF 04" );    // sub edi,4
			break;
		case OP_DIVU:
			EmitString( "8B 47 FC" );    // mov eax,dword ptr [edi-4]
			EmitString( "33 D2" );       // xor edx, edx
			EmitString( "F7 37" );       // div dword ptr [edi]
			EmitString( "89 47 FC" );    // mov dword ptr [edi-4],eax
			EmitString( "83 EF 04" );    // sub edi,4
			break;
		case OP_MODI:
			EmitString( "8B 47 FC" );    // mov eax,dword ptr [edi-4]
			EmitString( "99" );          // cdq
			EmitString( "F7 3F" );       // idiv dword ptr [edi]
			EmitString( "89 57 FC" );    // mov dword ptr [edi-4],edx
			EmitString( "83 EF 04" );    // sub edi,4
			break;
		case OP_MODU:
			EmitString( "8B 47 FC" );    // mov eax,dword ptr [edi-4]
			EmitString( "33 D2" );       // xor edx, edx
			EmitString( "F7 37" );       // div dword ptr [edi]
			EmitString( "89 57 FC" );    // mov dword ptr [edi-4],edx
			EmitString( "83 EF 04" );    // sub edi,4
			break;
		case OP_MULI:
			EmitString( "8B 47 FC" );    // mov eax,dword ptr [edi-4]
			EmitString( "F7 2F" );       // imul dword ptr [edi]
			EmitString( "89 47 FC" );    // mov dword ptr [edi-4],eax
			EmitString( "83 EF 04" );    // sub edi,4
			break;
		case OP_MULU:
			EmitString( "8B 47 FC" );    // mov eax,dword ptr [edi-4]
			EmitString( "F7 27" );       // mul dword ptr [edi]
			EmitString( "89 47 FC" );    // mov dword ptr [edi-4],eax
			EmitString( "83 EF 04" );    // sub edi,4
			break;
		case OP_BAND:
			EmitString( "8B 07" );       // mov eax, dword ptr [edi]
			EmitString( "21 47 FC" );    // and dword ptr [edi-4],eax
			EmitString( "83 EF 04" );    // sub edi,4
			break;
		case OP_BOR:
			EmitString( "8B 07" );       // mov eax, dword ptr [edi]
			EmitString( "09 47 FC" );    // or dword ptr [edi-4],eax
			EmitString( "83 EF 04" );    // sub edi,4
			break;
		case OP_BXOR:
			EmitString( "8B 07" );       // mov eax, dword ptr [edi]
			EmitString( "31 47 FC" );    // xor dword ptr [edi-4],eax
			EmitString( "83 EF 04" );    // sub edi,4
			break;
		case OP_BCOM:
			EmitString( "F7 17" );       // not dword ptr [edi]
			break;
		case OP_LSH:
			EmitString( "8B 0F" );       // mov ecx, dword ptr [edi]
			EmitString( "D3 67 FC" );    // shl dword ptr [edi-4], cl
			EmitString( "83 EF 04" );    // sub edi,4
			break;
		case OP_RSHI:
			EmitString( "8B 0F" );       // mov ecx, dword ptr [edi]
			EmitString( "D3 7F FC" );    // sar dword ptr [edi-4], cl
			EmitString( "83 EF 04" );    // sub edi,4
			break;
		case OP_RSHU:
			EmitString( "8B 0F" );       // mov ecx, dword ptr [edi]
			EmitString( "D3 6F FC" );    // shr dword ptr [edi-4], cl
			EmitString( "83 EF 04" );    // sub edi,4
			break;
		case OP_NEGF:
			EmitString( "D9 07" );       // fld dword ptr [edi]
			EmitString( "D9 E0" );       // fchs
			EmitString( "D9 1F" );       // fstp dword ptr [edi]
			break;
		case OP_ADDF:
			EmitString( "83 EF 04" );    // sub edi,4
			EmitString( "D9 07" );       // fld dword ptr [edi]
			EmitString( "D8 47 04" );    // fadd dword ptr [edi+4]
			EmitString( "D9 1F" );       // fstp dword ptr [edi]
			break;
		case OP_SUBF:
			EmitString( "83 EF 04" );    // sub edi,4
			EmitString( "D9 07" );       // fld dword ptr [edi]
			EmitString( "D8 67 04" );    // fsub dword ptr [edi+4]
			EmitString( "D9 1F" );       // fstp dword ptr [edi]
			break;
		case OP_DIVF:
			EmitString( "83 EF 04" );    // sub edi,4
			EmitString( "D9 07" );       // fld dword ptr [edi]
			EmitString( "D8 77 04" );    // fdiv dword ptr [edi+4]
			EmitString( "D9 1F" );       // fstp dword ptr [edi]
			break;
		case OP_MULF:
			EmitString( "83 EF 04" );    // sub edi,4
			EmitString( "D9 07" );       // fld dword ptr [edi]
			EmitString( "D8 4f 04" );    // fmul dword ptr [edi+4]
			EmitString( "D9 1F" );       // fstp dword ptr [edi]
			break;
		case OP_CVIF:
			EmitString( "DB 07" );       // fild dword ptr [edi]
			EmitString( "D9 1F" );       // fstp dword ptr [edi]
			break;
		case OP_CVFI:
			EmitString( "D9 07" );       // fld dword ptr [edi]
#if 1
			// not IEEE complient, but simple and fast
			EmitString( "DB 1F" );       // fistp dword ptr [edi]
#else
			// call the library conversion function
			EmitString( "FF 15" );       // call ftolPtr
			Emit4( (int)&ftolPtr );
			EmitString( "89 07" );       // mov dword ptr [edi], eax
#endif
			break;
		case OP_SEX8:
			EmitString( "0F BE 07" );    // movsx eax, byte ptr [edi]
			EmitString( "89 07" );       // mov dword ptr [edi], eax
			break;
		case OP_SEX16:
			EmitString( "0F BF 07" );    // movsx eax, word ptr [edi]
			EmitString( "89 07" );       // mov dword ptr [edi], eax
			break;

		case OP_BLOCK_COPY:
			// FIXME: range check
			EmitString( "56" );          // push esi
			EmitString( "57" );          // push edi
			EmitString( "8B 37" );       // mov esi,[edi]
			EmitString( "8B 7F FC" );    // mov edi,[edi-4]
			EmitString( "B9" );          // mov ecx,0x12345678
			Emit4( Constant4() >> 2 );
			EmitString( "B8" );          // mov eax, datamask
			Emit4( vm->dataMask );
			EmitString( "BB" );          // mov ebx, database
			Emit4( (int)vm->dataBase );
			EmitString( "23 F0" );       // and esi, eax
			EmitString( "03 F3" );       // add esi, ebx
			EmitString( "23 F8" );       // and edi, eax
			EmitString( "03 FB" );       // add edi, ebx
			EmitString( "F3 A5" );       // rep movsd
			EmitString( "5F" );          // pop edi
			EmitString( "5E" );          // pop esi
			EmitString( "83 EF 08" );    // sub	edi,8
			break;

		case OP_JUMP:
			EmitString( "83 EF 04" );    // sub edi,4
			EmitString( "8B 47 04" );    // mov eax,dword ptr [edi+4]
			// FIXME: range check
			EmitString( "FF 24 85" );    // jmp dword ptr [instructionPointers + eax * 4]
			Emit4( (int)vm->instructionPointers );
			break;
		default:
			Com_Error( ERR_DROP, "VM_CompileX86: bad opcode %i at offset %i", op, pc );
		}

	}

	// copy to an exact size buffer on the hunk
	vm->codeLength = compiledOfs;
	vm->codeBase = Hunk_Alloc( compiledOfs, h_low );
	memcpy( vm->codeBase, buf, compiledOfs );
	Z_Free( buf );
	Com_Printf( "VM file %s compiled to %i bytes of code\n", vm->name, compiledOfs );

	// offset all the instruction pointers for the new location
	for ( i = 0 ; i < header->instructionCount ; i++ ) {
		vm->instructionPointers[i] += (int)vm->codeBase;
	}

#if 0 // ndef _WIN32
	  // Must make the newly generated code executable
	{
		int r;
		unsigned long addr;
		int psize = getpagesize();

		addr = ( (int)vm->codeBase & ~( psize - 1 ) ) - psize;

		r = mprotect( (char*)addr, vm->codeLength + (int)vm->codeBase - addr + psize,
					  PROT_READ | PROT_WRITE | PROT_EXEC );

		if ( r < 0 ) {
			Com_Error( ERR_FATAL, "mprotect failed to change PROT_EXEC" );
		}
	}
#endif

}

/*
==============
VM_CallCompiled

This function is called directly by the generated code
==============
*/
int VM_CallCompiled( vm_t *vm, int *args ) {
	int stack[1024];
	int programCounter;
	int programStack;
	int stackOnEntry;
	byte    *image;
	void    *entryPoint;
	void    *opStack;
	int     *oldInstructionPointers;

	oldInstructionPointers = instructionPointers;

	currentVM = vm;
	instructionPointers = vm->instructionPointers;

	// interpret the code
	vm->currentlyInterpreting = qtrue;

	// we might be called recursively, so this might not be the very top
	programStack = vm->programStack;
	stackOnEntry = programStack;

	// set up the stack frame
	image = vm->dataBase;

	programCounter = 0;

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
	*(int *)&image[ programStack + 4 ] = 0; // return stack
	*(int *)&image[ programStack ] = -1;    // will terminate the loop on return

	// off we go into generated code...
	entryPoint = vm->codeBase;
	opStack = &stack;

#ifdef _WIN32
	__asm  {
		pushad
		mov esi, programStack;
		mov edi, opStack
		call entryPoint
		mov programStack, esi
		mov opStack, edi
		popad
	}
#else
	{
		static int memProgramStack;
		static void *memOpStack;
		static void *memEntryPoint;

		memProgramStack = programStack;
		memOpStack      = opStack;
		memEntryPoint   = entryPoint;

		__asm__( "	pushal				\r\n"\
				 "	movl %0,%%esi		\r\n"\
				 "	movl %1,%%edi		\r\n"\
				 "	call *%2			\r\n"\
				 "	movl %%esi,%0		\r\n"\
				 "	movl %%edi,%1		\r\n"\
				 "	popal				\r\n"\
				 : "=m" ( memProgramStack ), "=m" ( memOpStack ) \
				 : "m" ( memEntryPoint ), "0" ( memProgramStack ), "1" ( memOpStack ) \
				 : "si", "di" \
				 );

		programStack = memProgramStack;
		opStack      = memOpStack;
	}
#endif

	if ( opStack != &stack[1] ) {
		Com_Error( ERR_DROP, "opStack corrupted in compiled code" );
	}
	if ( programStack != stackOnEntry - 48 ) {
		Com_Error( ERR_DROP, "programStack corrupted in compiled code" );
	}

	vm->programStack = stackOnEntry;

	// in case we were recursively called by another vm
	instructionPointers = oldInstructionPointers;

	return *(int *)opStack;
}


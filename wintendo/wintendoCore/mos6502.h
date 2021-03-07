#pragma once

#include <map>
#include <string>
#include <sstream>
#include <vector>
#include <assert.h>
#include "common.h"
#include "serializer.h"
#include "debug.h"

class	Cpu6502;
struct	OpCodeMap;
struct	AddrMapTuple;
struct	InstructionMapTuple;
struct	DisassemblerMapTuple;
class	wtSystem;
struct	cpuAddrInfo_t;

enum class opType_t : uint8_t
{
	ADC, AND, ASL, BCC, BCS, BEQ, BIT, BMI, BNE, BPL, BRK, BVC, BVS, CLC,
	CLD, CLI, CLV, CMP, CPX, CPY, DEC, DEX, DEY, EOR, INC, INX, INY, JMP,
	JSR, LDA, LDX, LDY, LSR, NOP, ORA, PHA, PHP, PLA, PLP, ROL, ROR, RTI,
	RTS, SBC, SEC, SED, SEI, STA, STX, STY, TAX, TAY, TSX, TXA, TXS, TYA,
	JMPI, Illegal, SKB, SKW, LAX, SAX, DCP, ISB, SLO, RLA, SRE, RRA,
};

enum class addrMode_t : uint8_t
{
	None,
	Absolute,
	Zero,
	Immediate,
	IndexedIndirect,
	IndirectIndexed,
	Accumulator,
	IndexedAbsoluteX,
	IndexedAbsoluteY,
	IndexedZeroX,
	IndexedZeroY,
	Jmp,
	JmpIndirect,
	Jsr,
	Return,
	Branch,
};

typedef void( Cpu6502::* OpCodeFn )();
struct opInfo_t
{
	OpCodeFn	func;
	const char*	mnemonic;
	opType_t	type;
	addrMode_t	addrMode;
	uint8_t		operands;
	uint8_t		baseCycles;
	uint8_t		pcInc;
	bool		illegal;
};

#define OP_DECL( name )									template <class AddrModeT>												\
														void name();

#define OP_DEF( name )									template <class AddrModeT>												\
														void name()

#define ADDR_MODE_DECL( name )							struct addrMode##name													\
														{																		\
															static const addrMode_t addrMode = addrMode_t::##name;				\
															Cpu6502& cpu;														\
															addrMode##name( Cpu6502& cpui ) : cpu( cpui ) {};					\
															inline void operator()( cpuAddrInfo_t& addrInfo );					\
														};

#define ADDR_MODE_DEF( name )							void Cpu6502::addrMode##name::operator() ( cpuAddrInfo_t& addrInfo )

#define _OP_ADDR( num, name, addrFunc, addrressMode, ops, advance, cycles, isIllegal ) {										\
															opLUT[num].mnemonic		= #name;									\
															opLUT[num].type			= opType_t::##name;							\
															opLUT[num].addrMode		= addrMode_t::##addrressMode;				\
															opLUT[num].operands		= ops;										\
															opLUT[num].baseCycles	= cycles;									\
															opLUT[num].pcInc		= advance;									\
															opLUT[num].func			= &Cpu6502::##name<addrMode##addrFunc>;		\
															opLUT[num].illegal		= isIllegal;								\
														}
#define OP_ADDR( num, name, addrMode, ops, cycles )		_OP_ADDR( num,	name,	addrMode,	addrMode,	ops,	ops,	cycles,	false )
#define ILLEGAL( num, name, addrMode, ops, cycles )		_OP_ADDR( num,	name,	addrMode,	addrMode,	ops,	ops,	cycles,	true )
#define OP_JUMP( num, name, addrMode, ops, cycles )		_OP_ADDR( num,	name,	None,		addrMode,	ops,	0,		cycles,	false )


enum statusBit_t
{
	STATUS_NONE			= 0x00,
	STATUS_ALL			= 0xFF,
	STATUS_CARRY		= ( 1 << 0 ),
	STATUS_ZERO			= ( 1 << 1 ),
	STATUS_INTERRUPT	= ( 1 << 2 ),
	STATUS_DECIMAL		= ( 1 << 3 ),
	STATUS_BREAK		= ( 1 << 4 ),
	STATUS_UNUSED		= ( 1 << 5 ),
	STATUS_OVERFLOW		= ( 1 << 6 ),
	STATUS_NEGATIVE		= ( 1 << 7 ),
};


union statusReg_t
{
	struct semantic
	{
		uint8_t c : 1;	// Carry
		uint8_t z : 1;	// Zero
		uint8_t i : 1;	// Interrupt
		uint8_t d : 1;	// Decimal

		uint8_t u : 1;	// Unused
		uint8_t b : 1;	// Break
		uint8_t v : 1;	// Overflow
		uint8_t n : 1;	// Negative
	} bit;

	uint8_t byte;
};


struct cpuAddrInfo_t
{
	cpuAddrInfo_t() : addr( ~0x00 ), targetAddr( ~0x00 ), offset( 0 ), value( 0 ) {}

	cpuAddrInfo_t( const uint32_t _addr, const uint32_t _targetAddr, const uint16_t _offset, const uint8_t _value ) :
		addr( _addr ), targetAddr( _targetAddr ), offset( _offset ), value( _value ) {}

	uint32_t	addr;
	uint32_t	targetAddr;
	uint16_t	offset;
	uint8_t		value;
};

class OpDebugInfo;
class wtLog;

class Cpu6502
{
public:
	static const uint32_t InvalidAddress	= ~0x00;
	static const uint32_t NumInstructions	= 256;

	uint16_t			nmiVector;
	uint16_t			irqVector;
	uint16_t			resetVector;

	wtSystem*			system;
	cpuCycle_t			cycle;

#if DEBUG_ADDR == 1
	std::stringstream	debugAddr;
	std::ofstream		logFile;
	bool				logToFile = false;
	int32_t				logFrameCount = 0;
	int32_t				logFrameTotal = 0;
#endif
	bool				resetLog;
	wtLog				dbgLog;

	// TODO: move to system
	mutable uint16_t	irqAddr;
	mutable bool		interruptRequestNMI;
	mutable bool		interruptRequest;
	mutable bool		oamInProcess;
	mutable bool		dmcTransfer;

	uint8_t				X;
	uint8_t				Y;
	uint8_t				A;
	uint8_t				SP;
	statusReg_t			P;
	uint16_t			PC;

	opInfo_t			opLUT[NumInstructions];

private:
	// TODO: make 'execution state' struct which has per op variables with short lifetimes such as these
	cpuCycle_t			instructionCycles;
	uint8_t				opCode;
	bool				halt;

	cpuCycle_t			dbgStartCycle;
	cpuCycle_t			dbgTargetCycle;
	masterCycles_t		dbgSysStartCycle;
	masterCycles_t		dbgSysTargetCycle;

public:
	void Reset()
	{
		PC = resetVector;

		X = 0;
		Y = 0;
		A = 0;
		SP = 0xFD;

		P.byte = 0;
		P.bit.i = 1;
		P.bit.b = 1;

		instructionCycles = cpuCycle_t( 0 );
		cycle = cpuCycle_t( 7 ); // FIXME: +7 is a hack to match test log, +21 on PPU

		interruptRequestNMI = false;
		interruptRequest = false;
		oamInProcess = false;
		dmcTransfer = false;

		halt = false;

		resetLog = false;
		dbgLog.Reset(0);
	}

	Cpu6502()
	{
		resetLog = false;
		Reset();
		BuildOpLUT();
	}

	bool Step( const cpuCycle_t& nextCycle );
	void RegisterSystem( wtSystem* sys );
	bool IsLogOpen() const;

	void Serialize( Serializer& serializer, const serializeMode_t mode );

private:
	// All ops included here
	#include "mos6502_ops.h"

	// These functions require system memory.
	ADDR_MODE_DECL( None )
	ADDR_MODE_DECL( Absolute )
	ADDR_MODE_DECL( Zero )
	ADDR_MODE_DECL( Immediate )
	ADDR_MODE_DECL( IndexedIndirect )
	ADDR_MODE_DECL( IndirectIndexed )
	ADDR_MODE_DECL( Accumulator )
	ADDR_MODE_DECL( IndexedAbsoluteX )
	ADDR_MODE_DECL( IndexedAbsoluteY )
	ADDR_MODE_DECL( IndexedZeroX )
	ADDR_MODE_DECL( IndexedZeroY )
	uint16_t	JumpImmediateAddr( const uint16_t addr );

	cpuCycle_t	Exec();

	static bool	CheckSign( const uint16_t checkValue );
	static bool	CheckCarry( const uint16_t checkValue );
	static bool	CheckZero( const uint16_t checkValue );
	static bool	CheckOverflow( const uint16_t src, const uint16_t temp, const uint8_t finalValue );

	void		NMI( const uint16_t addr );
	void		IRQ();

	void		IndexedAbsolute( const uint8_t& reg, cpuAddrInfo_t& addrInfo );
	void		IndexedZero( const uint8_t& reg, cpuAddrInfo_t& addrInfo );

	void		Push( const uint8_t value );
	void		PushWord( const uint16_t value );
	uint8_t		Pull();
	uint16_t	PullWord();

	void		AdvancePC( const uint16_t places );
	uint8_t		ReadOperand( const uint16_t offset ) const;
	uint16_t	ReadAddressOperand() const;

	void		SetAluFlags( const uint16_t value );

	uint16_t	CombineIndirect( const uint8_t lsb, const uint8_t msb, const uint32_t wrap );

	uint8_t		AddressCrossesPage( const uint16_t address, const uint16_t offset );
	uint8_t		Branch( const bool takeBranch );
	
	template <class AddrFunctor>
	uint8_t		Read();

	template <class AddrFunctor>
	void		Write( const uint8_t value );

	cpuCycle_t	OpLookup( const uint16_t instrBegin, const uint8_t opCode );
};
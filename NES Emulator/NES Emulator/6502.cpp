#include "stdafx.h"
#include <iostream>
#include <iomanip>
#include <Windows.h>
#include <chrono>
#include <thread>
#include <fstream>
#include <string>
#include <sstream>
#include <assert.h>
#include <map>
#include <bitset>
#include "common.h"
#include "debug.h"
#include "6502.h"
#include "NesSystem.h"

using namespace std;

void RegistersToString( const Cpu6502& cpu, string& regStr )
{
	stringstream sStream;

	sStream << uppercase << "A:" << setfill( '0' ) << setw( 2 ) << hex << static_cast<int>( cpu.A ) << setw( 1 ) << " ";
	sStream << uppercase << "X:" << setfill( '0' ) << setw( 2 ) << hex << static_cast<int>( cpu.X ) << setw( 1 ) << " ";
	sStream << uppercase << "Y:" << setfill( '0' ) << setw( 2 ) << hex << static_cast<int>( cpu.Y ) << setw( 1 ) << " ";
	sStream << uppercase << "P:" << setfill( '0' ) << setw( 2 ) << hex << static_cast<int>( cpu.P.byte ) << setw( 1 ) << " ";
	sStream << uppercase << "SP:" << setfill( '0' ) << setw( 2 ) << hex << static_cast<int>( cpu.SP ) << setw( 1 );
	//sStream << uppercase << " CYC:" << setw( 3 ) << "0\0";

	regStr = sStream.str();
}


OP_DEF( SEC )
{
	P.bit.c = 1;

	return 0;
}


OP_DEF( SEI )
{
	P.bit.i = 1;

	return 0;
}


OP_DEF( SED )
{
	P.bit.d = 1;

	return 0;
}


OP_DEF( CLC )
{
	P.bit.c = 0;

	return 0;
}


OP_DEF( CLI )
{
	P.bit.i = 0;

	return 0;
}


OP_DEF( CLV )
{
	P.bit.v = 0;

	return 0;
}


OP_DEF( CLD )
{
	P.bit.d = 0;

	return 0;
}


OP_DEF( CMP )
{
	const uint16_t result = ( A - Read( params ) );

	P.bit.c = !CheckCarry( result );
	SetAluFlags( result );

	return 0;
}


OP_DEF( CPX )
{
	const uint16_t result = ( X - Read( params ) );

	P.bit.c = !CheckCarry( result );
	SetAluFlags( result );

	return 0;
}


OP_DEF( CPY )
{
	const uint16_t result = ( Y - Read( params ) );

	P.bit.c = !CheckCarry( result );
	SetAluFlags( result );

	return 0;
}


OP_DEF( LDA )
{
	A = Read( params );

	SetAluFlags( A );

	return 0;
}

OP_DEF( LDX )
{
	X = Read( params );

	SetAluFlags( X );

	return 0;
}


OP_DEF( LDY )
{
	Y = Read( params );

	SetAluFlags( Y );

	return 0;
}


OP_DEF( STA )
{
	Write( params, A );

	return 0;
}


OP_DEF( STX )
{
	Write( params, X );

	return 0;
}


OP_DEF( STY )
{
	Write( params, Y );

	return 0;
}


OP_DEF( TXS )
{
	SP = X;

	return 0;
}


OP_DEF( TXA )
{
	A = X;
	SetAluFlags( A );

	return 0;
}


OP_DEF( TYA )
{
	A = Y;
	SetAluFlags( A );

	return 0;
}


OP_DEF( TAX )
{
	X = A;
	SetAluFlags( X );

	return 0;
}


OP_DEF( TAY )
{
	Y = A;
	SetAluFlags( Y );

	return 0;
}


OP_DEF( TSX )
{
	X = SP;
	SetAluFlags( X );

	return 0;
}


OP_DEF( ADC )
{
	// http://nesdev.com/6502.txt, "INSTRUCTION OPERATION - ADC"
	const uint8_t M = Read( params );
	const uint16_t src = A;
	const uint16_t carry = ( P.bit.c ) ? 1 : 0;
	const uint16_t temp = A + M + carry;

	A = ( temp & 0xFF );

	P.bit.z = CheckZero( temp );
	P.bit.v = CheckOverflow( M, temp, A );
	SetAluFlags( A );

	P.bit.c = ( temp > 0xFF );

	return 0;
}


OP_DEF( SBC )
{
	const uint8_t& M = Read( params );
	const uint16_t carry = ( P.bit.c ) ? 0 : 1;
	const uint16_t result = A - M - carry;

	SetAluFlags( result );

	P.bit.v = ( CheckSign( A ^ result ) && CheckSign( A ^ M ) );
	P.bit.c = !CheckCarry( result );

	A = result & 0xFF;

	return 0;
}


OP_DEF( INX )
{
	++X;
	SetAluFlags( X );

	return 0;
}


OP_DEF( INY )
{
	++Y;
	SetAluFlags( Y );

	return 0;
}


OP_DEF( DEX )
{
	--X;
	SetAluFlags( X );

	return 0;
}


OP_DEF( DEY )
{
	--Y;
	SetAluFlags( Y );

	return 0;
}


OP_DEF( INC )
{
	const uint8_t result = Read( params ) + 1;

	Write( params, result );

	SetAluFlags( result );

	return 0;
}


OP_DEF( DEC )
{
	const uint8_t result = Read( params ) - 1;

	Write( params, result );

	SetAluFlags( result );

	return 0;
}


OP_DEF( PHP )
{
	Push( P.byte | STATUS_UNUSED | STATUS_BREAK );

	return 0;
}


OP_DEF( PHA )
{
	Push( A );

	return 0;
}


OP_DEF( PLA )
{
	A = Pull();

	SetAluFlags( A );

	return 0;
}


OP_DEF( PLP )
{
	// https://wiki.nesdev.com/w/index.php/Status_flags
	const uint8_t status = ~STATUS_BREAK & Pull();
	P.byte = status | ( P.byte & STATUS_BREAK ) | STATUS_UNUSED;

	return 0;
}


OP_DEF( NOP )
{
	return 0;
}


OP_DEF( ASL )
{
	const uint8_t& M = Read( params );

	P.bit.c = !!( M & 0x80 );
	Write( params, M << 1 );
	SetAluFlags( M );

	return 0;
}


OP_DEF( LSR )
{
	const uint8_t& M = Read( params );

	P.bit.c = ( M & 0x01 );
	Write( params, M >> 1 );
	SetAluFlags( M );

	return 0;
}


OP_DEF( AND )
{
	A &= Read( params );

	SetAluFlags( A );

	return 0;
}


OP_DEF( BIT )
{
	const uint8_t& M = Read( params );

	P.bit.z = !( A & M );
	P.bit.n = CheckSign( M );
	P.bit.v = !!( M & 0x40 );

	return 0;
}


OP_DEF( EOR )
{
	A ^= Read( params );

	SetAluFlags( A );

	return 0;
}


OP_DEF( ORA )
{
	A |= Read( params );

	SetAluFlags( A );

	return 0;
}


OP_DEF( JMP )
{
	PC = Combine( params.param0, params.param1 );

	DEBUG_ADDR_JMP

	return 0;
}


OP_DEF( JMPI )
{
	const uint16_t addr0 = Combine( params.param0, params.param1 );

	// Hardware bug - http://wiki.nesdev.com/w/index.php/Errata
	if ( ( addr0 & 0xff ) == 0xff )
	{
		const uint16_t addr1 = Combine( 0x00, params.param1 );

		PC = ( Combine( system->GetMemory( addr0 ), system->GetMemory( addr1 ) ) );
	}
	else
	{
		PC = ( Combine( system->GetMemory( addr0 ), system->GetMemory( addr0 + 1 ) ) );
	}

	DEBUG_ADDR_JMPI

		return 0;
}


OP_DEF( JSR )
{
	uint16_t retAddr = PC - 1;

	Push( ( retAddr >> 8 ) & 0xFF );
	Push( retAddr & 0xFF );

	PC = Combine( params.param0, params.param1 );

	DEBUG_ADDR_JSR

		return 0;
}


OP_DEF( BRK )
{
	P.bit.b = 1;

	interruptTriggered = true;

	return 0;
}


OP_DEF( RTS )
{
	const uint8_t loByte = Pull();
	const uint8_t hiByte = Pull();

	PC = 1 + Combine( loByte, hiByte );

	return 0;
}


OP_DEF( RTI )
{
	PLP( params );

	const uint8_t loByte = Pull();
	const uint8_t hiByte = Pull();

	PC = Combine( loByte, hiByte );

	return 0;
}


OP_DEF( BMI )
{
	return Branch( params, ( P.bit.n ) );
}


OP_DEF( BVS )
{
	return Branch( params, ( P.bit.v ) );
}


OP_DEF( BCS )
{
	return Branch( params, ( P.bit.c ) );
}


OP_DEF( BEQ )
{
	return Branch( params, ( P.bit.z ) );
}


OP_DEF( BPL )
{
	return Branch( params, !( P.bit.n ) );
}


OP_DEF( BVC )
{
	return Branch( params, !( P.bit.v ) );
}


OP_DEF( BCC )
{
	return Branch( params, !( P.bit.c ) );
}


OP_DEF( BNE )
{
	return Branch( params, !( P.bit.z ) );
}


OP_DEF( ROL )
{
	uint16_t temp = Read( params ) << 1;
	temp = ( P.bit.c ) ? temp | 0x0001 : temp;

	P.bit.c = CheckCarry( temp );

	temp &= 0xFF;

	SetAluFlags( temp );

	Read( params ) = static_cast< uint8_t >( temp & 0xFF );

	return 0;
}


OP_DEF( ROR )
{
	uint16_t temp = ( P.bit.c ) ? Read( params ) | 0x0100 : Read( params );

	P.bit.c = ( temp & 0x01 );

	temp >>= 1;

	SetAluFlags( temp );

	Read( params ) = static_cast< uint8_t >( temp & 0xFF );

	return 0;
}


OP_DEF( Illegal )
{
	//assert( 0 );

	return 0;
}


ADDR_MODE_DEF( IndexedIndirect )
{
	const uint8_t targetAddress = ( params.param0 + X );
	address = CombineIndirect( targetAddress, 0x00, NesSystem::ZeroPageWrap );

	uint8_t& value = system->GetMemory( address );

	DEBUG_ADDR_INDEXED_INDIRECT

	return value;
}


ADDR_MODE_DEF( IndirectIndexed )
{
	address = CombineIndirect( params.param0, 0x00, NesSystem::ZeroPageWrap );

	const uint16_t offset = ( address + Y ) % NesSystem::MemoryWrap;

	uint8_t& value = system->GetMemory( offset );

	AddPageCrossCycles( address );

	DEBUG_ADDR_INDIRECT_INDEXED

	return value;
}


ADDR_MODE_DEF( Absolute )
{
	address = Combine( params.param0, params.param1 );

	uint8_t& value = system->GetMemory( address );

	DEBUG_ADDR_ABS

	return value;
}


ADDR_MODE_DEF( IndexedAbsoluteX )
{
	return IndexedAbsolute( params, address, X );
}


ADDR_MODE_DEF( IndexedAbsoluteY )
{
	return IndexedAbsolute( params, address, Y );
}


ADDR_MODE_DEF( Zero )
{
	const uint16_t targetAddresss = Combine( params.param0, 0x00 );

	address = ( targetAddresss % NesSystem::ZeroPageWrap );

	uint8_t& value = system->GetMemory( address );

	DEBUG_ADDR_ZERO

	return value;
}


ADDR_MODE_DEF( IndexedZeroX )
{
	return IndexedZero( params, address, X );
}


ADDR_MODE_DEF( IndexedZeroY )
{
	return IndexedZero( params, address, Y );
}


ADDR_MODE_DEF( Immediate )
{
	// This is a bit dirty, but necessary to maintain uniformity, asserts have been placed at lhs usages for now
	uint8_t& value = const_cast< InstrParams& >( params ).param0;

	address = Cpu6502::InvalidAddress;

	DEBUG_ADDR_IMMEDIATE

	return value;
}


ADDR_MODE_DEF( Accumulator )
{
	address = Cpu6502::InvalidAddress;

	DEBUG_ADDR_ACCUMULATOR

	return A;
}


void Cpu6502::Push( const uint8_t value )
{
	system->GetStack() = value;
	SP--;
}


void Cpu6502::PushByte( const uint16_t value )
{
	Push( ( value >> 8 ) & 0xFF );
	Push( value & 0xFF );
}


inline uint8_t Cpu6502::Pull()
{
	SP++;
	return system->GetStack();
}


uint16_t Cpu6502::PullWord()
{
	const uint8_t loByte = Pull();
	const uint8_t hiByte = Pull();

	return Combine( loByte, hiByte );
}


inline uint8_t Cpu6502::Branch( const InstrParams& params, const bool takeBranch )
{
	const uint16_t branchedPC = PC + static_cast< int8_t >( params.param0 );

	uint8_t cycles = 0;

	if ( takeBranch )
	{
		PC = branchedPC;
	}
	else
	{
		++cycles;
	}

	DEBUG_ADDR_BRANCH

	return ( cycles + AddPageCrossCycles( branchedPC ) );
}


inline void Cpu6502::SetAluFlags( const uint16_t value )
{
	P.bit.z = CheckZero( value );
	P.bit.n = CheckSign( value );
}


inline bool Cpu6502::CheckSign( const uint16_t checkValue )
{
	return ( checkValue & 0x0080 );
}


inline bool Cpu6502::CheckCarry( const uint16_t checkValue )
{
	return ( checkValue > 0x00ff );
}


inline bool Cpu6502::CheckZero( const uint16_t checkValue )
{
	return ( checkValue == 0 );
}


inline bool Cpu6502::CheckOverflow( const uint16_t src, const uint16_t temp, const uint8_t finalValue )
{
	const uint8_t signedBound = 0x80;
	return CheckSign( finalValue ^ src ) && CheckSign( temp ^ src ) && !CheckCarry( temp );
}


inline uint8_t Cpu6502::AddPageCrossCycles( const uint16_t address )
{
	instructionCycles;
	return 0;
}


inline uint16_t Cpu6502::CombineIndirect( const uint8_t lsb, const uint8_t msb, const uint32_t wrap )
{
	const uint16_t address = Combine( lsb, msb );
	const uint8_t loByte = system->GetMemory( address % wrap );
	const uint8_t hiByte = system->GetMemory( ( address + 1 ) % wrap );
	const uint16_t value = Combine( loByte, hiByte );

	return value;
}


void Cpu6502::DebugIndexZero( const uint8_t& reg, const uint32_t address, const uint32_t targetAddresss )
{
#if DEBUG_ADDR == 1
	uint8_t& value = system->GetMemory( address );
	debugAddr.str( std::string() );
	debugAddr << uppercase << "$" << setw( 2 ) << hex << targetAddresss << ",";
	debugAddr << ( ( &reg == &X ) ? "X" : "Y" );
	debugAddr << setfill( '0' ) << " @ " << setw( 2 ) << hex << address;
	debugAddr << " = " << setw( 2 ) << hex << static_cast< uint32_t >( value );
#endif // #if DEBUG_ADDR == 1
}


inline uint8_t& Cpu6502::IndexedZero( const InstrParams& params, uint32_t& address, const uint8_t& reg )
{
	const uint16_t targetAddresss = Combine( params.param0, 0x00 );

	address = ( targetAddresss + reg ) % NesSystem::ZeroPageWrap;

	uint8_t& value = system->GetMemory( address );

	DebugIndexZero( reg, address, targetAddresss );

	return value;
}


inline uint8_t& Cpu6502::IndexedAbsolute( const InstrParams& params, uint32_t& address, const uint8_t& reg )
{
	const uint16_t targetAddresss = Combine( params.param0, params.param1 );

	address = ( targetAddresss + reg ) % NesSystem::MemoryWrap;

	uint8_t& value = system->GetMemory( address );

	AddPageCrossCycles( address );

	DEBUG_ADDR_INDEXED_ABS

	return value;
}


inline uint8_t Cpu6502::NMI()
{
	PushByte( PC - 1 );
	// http://wiki.nesdev.com/w/index.php/CPU_status_flag_behavior
	Push( P.byte | STATUS_BREAK );

	P.bit.i = 1;

	PC = nmiVector;

	return 0;
}


inline uint8_t Cpu6502::IRQ()
{
	return NMI();
}


inline uint8_t Cpu6502::PullProgramByte()
{
	return system->GetMemory( PC++ );
}


inline uint8_t Cpu6502::PeekProgramByte() const
{
	return system->GetMemory( PC );
}


inline cpuCycle_t Cpu6502::Exec()
{
#if DEBUG_ADDR == 1
	static bool enablePrinting = true;

	debugAddr.str( std::string() );
	string regStr;
	RegistersToString( *this, regStr );
#endif // #if DEBUG_ADDR == 1

	instructionCycles = cpuCycle_t( 0 );

	if ( oamInProcess )
	{

		// http://wiki.nesdev.com/w/index.php/PPU_registers#OAMDMA
		if ( ( cycle % 2 ) == cpuCycle_t( 0 ) )
			instructionCycles += cpuCycle_t( 514 );
		else
			instructionCycles += cpuCycle_t( 513 );

		oamInProcess = false;

		return instructionCycles;
	}

	const uint16_t instrBegin = PC;

#if DEBUG_MODE == 1
	if ( PC == forceStopAddr )
	{
		forceStop = true;
		return cpuCycle_t( 0 );
	}
#endif // #if DEBUG_MODE == 1

	uint8_t curbyte = system->GetMemory( instrBegin );

	PC++;

	InstrParams params;

	if ( resetTriggered )
	{
	}

	if ( interruptTriggered )
	{
		NMI();

		interruptTriggered = false;

		//Exec();

		return cpuCycle_t( 0 );
	}

	const InstructionMapTuple& pair = InstructionMap[curbyte];

	const uint8_t operands = pair.operands;

	Instruction_depr instruction = pair.instr;

	params.getAddr = pair.addrFunc;
	params.cpu = this;

	if ( operands >= 1 )
	{
		params.param0 = system->GetMemory( PC );
	}

	if ( operands == 2 )
	{
		params.param1 = system->GetMemory( PC + 1 );
	}

	PC += operands;

	//if( curbyte == 0xA9 )
	{
	//	( *pair.addrFunctor )( params, *this );
	}
	//else
	{
		( this->*instruction )( params );
	}

	instructionCycles += cpuCycle_t( pair.cycles );

	DEBUG_CPU_LOG

	return instructionCycles;
}


bool Cpu6502::Step( const cpuCycle_t nextCycle )
{
	/*
	if ( interruptTriggered )
	{
	NMI();

	interruptTriggered = false;

	Exec();
	}*/

	while ( ( cycle < nextCycle ) && !forceStop )
	{
		cycle += cpuCycle_t( Exec() );
	}

	return !forceStop;
}
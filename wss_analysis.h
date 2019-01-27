#pragma once

#include "wss.h"
#include "pin.H"

#include <fstream>

namespace WSS { namespace Analysis {

void Init(const std::string& filepath);
void Fini();
void PrintWSS();

void RecordMemRead(void* addr);
void RecordMemWrite(void* addr);

ADDRINT MaxCapacity();
void EarlyExit();

void SetRoutinePtr(RoutineRecord* routine);

void CountInstruction(UINT32 c);
void CountPtr(UINT64 *counter);

extern std::vector<RoutineRecord*> sRoutines; // good old vector of pointers - Pin's STL Port is pre-C++11, so no move semantics...
extern RoutineRecord* sCurrRoutine;
extern AddrRecord *sReads;
extern AddrRecord *sWrites;
extern uint64_t sInsn;
extern uint64_t sRidx;
extern uint64_t sWidx;
extern std::ofstream sOutputFile;

inline void RecordMemRead(void* addr)
{
	AddrRecord& record = sReads[sRidx++];
	record.mRoutine = sCurrRoutine;
	record.mAddr = addr;
}

inline void RecordMemWrite(void* addr)
{
	AddrRecord& record = sWrites[sWidx++];
	record.mRoutine = sCurrRoutine;
	record.mAddr = addr;
}

inline void CountInstruction(UINT32 c)
{
	sInsn += c;
}

inline ADDRINT MaxCapacity()
{
	return sInsn == sMaxInstructions || sWidx == sMaxRecords || sRidx == sMaxRecords;
}

inline void CountPtr(UINT64 *counter)
{
	++(*counter);
}

inline void SetRoutinePtr(RoutineRecord* routine)
{
	sCurrRoutine = routine;
}

}}

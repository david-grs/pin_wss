#include "wss_instr.h"
#include "wss_analysis.h"

namespace WSS { namespace Instrumentation {

void Init()
{
	TRACE_AddInstrumentFunction(Trace, 0);
	INS_AddInstrumentFunction(Instruction, 0);
	RTN_AddInstrumentFunction(Routine, 0);
	PIN_AddFiniFunction(Fini, 0);
}

void Fini(INT32, void*)
{
	Analysis::Fini();
	Analysis::PrintWSS();
}

void Trace(TRACE trace, void*)
{
	for (BBL bbl = TRACE_BblHead(trace); BBL_Valid(bbl); bbl = BBL_Next(bbl))
	{
		BBL_InsertCall(bbl, IPOINT_BEFORE, (AFUNPTR)Analysis::CountInstruction, IARG_UINT32, BBL_NumIns(bbl), IARG_END);
	}
}

void Instruction(INS ins, void*)
{
	const UINT32 memOperands = INS_MemoryOperandCount(ins);

	for (UINT32 memOp = 0; memOp < memOperands; memOp++)
	{
		if (INS_MemoryOperandIsRead(ins, memOp))
		{
			INS_InsertIfCall(ins, IPOINT_BEFORE, (AFUNPTR)Analysis::MaxCapacity, IARG_END);
			INS_InsertThenCall(ins, IPOINT_BEFORE, (AFUNPTR)Analysis::EarlyExit, IARG_END);

			INS_InsertPredicatedCall(
			    ins, IPOINT_BEFORE, (AFUNPTR)Analysis::RecordMemRead,
			    IARG_MEMORYOP_EA, memOp,
			    IARG_END);
		}

		if (INS_MemoryOperandIsWritten(ins, memOp))
		{
			INS_InsertIfCall(ins, IPOINT_BEFORE, (AFUNPTR)Analysis::MaxCapacity, IARG_END);
			INS_InsertThenCall(ins, IPOINT_BEFORE, (AFUNPTR)Analysis::EarlyExit, IARG_END);

			INS_InsertPredicatedCall(
			    ins, IPOINT_BEFORE, (AFUNPTR)Analysis::RecordMemWrite,
			    IARG_MEMORYOP_EA, memOp,
			    IARG_END);
		}
	}
}

void Routine(RTN rtn, void*)
{
	Analysis::sRoutines.push_back(new RoutineRecord{RTN_Name(rtn), RTN_Address(rtn)});
	RoutineRecord& routine = *Analysis::sRoutines.back();

	RTN_Open(rtn);
	RTN_InsertCall(rtn, IPOINT_BEFORE, (AFUNPTR)Analysis::CountPtr, IARG_PTR, &(routine.mCalls), IARG_END);

	for (INS ins = RTN_InsHead(rtn); INS_Valid(ins); ins = INS_Next(ins))
	{
		INS_InsertCall(
		    ins, IPOINT_BEFORE,
		    (AFUNPTR)Analysis::CountPtr, IARG_PTR, &(routine.mInstructions),
		    IARG_END);

		INS_InsertCall(
		    ins, IPOINT_BEFORE, (AFUNPTR)Analysis::SetRoutinePtr,
		    IARG_PTR, &routine,
		    IARG_END);
	}

	RTN_Close(rtn);
}

}}

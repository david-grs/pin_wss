#include "pin.H"

#include <cstdint>
#include <iostream>
#include <algorithm>
#include <unordered_set>
#include <sstream>

extern "C"
{
#include <unistd.h>
}

struct Record
{
	void* mIP = nullptr;
	void* mAddr = nullptr;
};

static const std::size_t MaxRecords = 256 * 1024 * 1024;
static const std::size_t MaxInstructions = 4000000000ULL;

Record *reads = nullptr;
Record *writes = nullptr;
std::size_t insn = 0;
std::size_t ridx = 0;
std::size_t widx = 0;
int stdoutfd;

void PrintWSS(const char* reason)
{
	std::ostringstream oss;
	oss << reason << std::endl
	    << insn << " instructions" << std::endl
	    << (ridx + widx) << " access" << std::endl
	    << ridx << " reads" << std::endl
	    << widx << " writes" << std::endl;

	std::tr1::unordered_set<void*> addrs;
	for (auto it = reads; it < reads + ridx; ++it)
	{
		addrs.insert((void*)(std::ptrdiff_t(it->mAddr) & (~63)));
	}
	for (auto it = writes; it < writes + widx; ++it)
	{
		addrs.insert((void*)(std::ptrdiff_t(it->mAddr) & (~63)));
	}

	oss << "WSS=" << ((addrs.size() * 64) / 1024) << "kB" << std::endl;
	const std::string str = oss.str();
	::write(stdoutfd, str.c_str(), str.size());
}

void EarlyExit()
{
	PrintWSS("early exit");
	PIN_Detach();
}

void RecordMemRead(void* ip, void* addr)
{
	Record& record = reads[ridx++];
	record.mIP = ip;
	record.mAddr = addr;
}

void RecordMemWrite(void* ip, void* addr)
{
	Record& record = writes[widx++];
	record.mIP = ip;
	record.mAddr = addr;
}

ADDRINT CountDown()
{
	return insn == MaxInstructions || widx == MaxRecords || ridx == MaxRecords;
}

void Instruction(INS ins, void*v)
{
	UINT32 memOperands = INS_MemoryOperandCount(ins);

	// Iterate over each memory operand of the instruction.
	for (UINT32 memOp = 0; memOp < memOperands; memOp++)
	{
		if (INS_MemoryOperandIsRead(ins, memOp))
		{
			INS_InsertIfCall(ins, IPOINT_BEFORE, (AFUNPTR)CountDown, IARG_END);
			INS_InsertThenCall(ins, IPOINT_BEFORE, (AFUNPTR)EarlyExit, IARG_END);

			INS_InsertPredicatedCall(
			    ins, IPOINT_BEFORE, (AFUNPTR)RecordMemRead,
			    IARG_INST_PTR,
			    IARG_MEMORYOP_EA, memOp,
			    IARG_END);
		}
		if (INS_MemoryOperandIsWritten(ins, memOp))
		{
			INS_InsertIfCall(ins, IPOINT_BEFORE, (AFUNPTR)CountDown, IARG_END);
			INS_InsertThenCall(ins, IPOINT_BEFORE, (AFUNPTR)EarlyExit, IARG_END);

			INS_InsertPredicatedCall(
			    ins, IPOINT_BEFORE, (AFUNPTR)RecordMemWrite,
			    IARG_INST_PTR,
			    IARG_MEMORYOP_EA, memOp,
			    IARG_END);
		}
	}
}

VOID CountInstruction(UINT32 c)
{
	insn += c;
}

VOID Trace(TRACE trace, VOID*)
{
	for (BBL bbl = TRACE_BblHead(trace); BBL_Valid(bbl); bbl = BBL_Next(bbl))
	{
		BBL_InsertCall(bbl, IPOINT_BEFORE, (AFUNPTR)CountInstruction, IARG_UINT32, BBL_NumIns(bbl), IARG_END);
	}
}

void Fini(INT32 /*code*/, void* /*v*/)
{
	PrintWSS("exit");
}

int main(int argc, char *argv[])
{
	if (PIN_Init(argc, argv))
	{
		PIN_ERROR("This Pintool evalutes the WSS (Working Set Size) of a process\n" + KNOB_BASE::StringKnobSummary() + "\n");
		return 1;
	}

	stdoutfd = ::dup(1);
	reads = new Record[MaxRecords];
	writes = new Record[MaxRecords];

	TRACE_AddInstrumentFunction(Trace, 0);
	INS_AddInstrumentFunction(Instruction, 0);
	PIN_AddFiniFunction(Fini, 0);

	PIN_StartProgram();
	return 0;
}

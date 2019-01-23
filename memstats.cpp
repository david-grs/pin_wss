#include "pin.H"

#include <cstdint>
#include <iostream>
#include <algorithm>
#include <deque>
#include <unordered_map>
#include <sstream>

extern "C"
{
#include <unistd.h>
}

struct RoutineRecord
{
	RoutineRecord(const std::string& name, ADDRINT addr) :
	    mName(name), mAddr(addr)
	{}

	std::string mName;
	ADDRINT mAddr = 0;
	uint64_t mInstructions = 0;
	uint64_t mCalls = 0;
};

struct AddrRecord
{
	RoutineRecord* mRoutine = nullptr;
	void* mAddr = nullptr;
};

static const std::size_t MaxRecords = 256 * 1024 * 1024;
static const std::size_t MaxInstructions = 4000000000ULL;

std::deque<RoutineRecord> routines;
AddrRecord *reads = nullptr;
AddrRecord *writes = nullptr;
uint64_t insn = 0;
uint64_t ridx = 0;
uint64_t widx = 0;
int stdoutfd;

void PrintMemCounters(const char* reason = "")
{
	struct MemCounters
	{
		RoutineRecord *mRoutine = nullptr;
		uint64_t mReads = 0;
		uint64_t mWrites = 0;
	};

	std::tr1::unordered_map<void*, MemCounters> counters;
	for (auto it = reads; it < reads + ridx; ++it)
	{
		counters[it->mRoutine].mReads += 1;
	}
	for (auto it = writes; it < writes + widx; ++it)
	{
		counters[it->mRoutine].mWrites += 1;
	}

	std::vector<MemCounters> stats;
	stats.reserve(counters.size());
	for (const auto& p : counters)
	{
		stats.push_back(MemCounters{(RoutineRecord*)p.first, p.second.mReads, p.second.mWrites});
	}

	std::sort(stats.begin(), stats.end(), [](const auto& lhs, const auto& rhs)
	{
		return (lhs.mReads + lhs.mWrites) > (rhs.mReads + rhs.mWrites);
	});

	std::ostringstream oss;
	oss << reason << std::endl;
	oss << std::setw(12) << std::right << "mem_reads"
	    << std::setw(12) << std::right << "mem_writes"
	    << std::setw(12) << std::right << "calls"
	    << std::setw(6) << " "
	    << std::left << "function" << std::endl;

	for (const auto& x : stats)
	{
		oss << std::setw(12) << std::right << x.mReads
		    << std::setw(12) << std::right << x.mWrites
		    << std::setw(12) << std::right << x.mRoutine->mCalls
		    << std::setw(6) << " "
		    << std::left << x.mRoutine->mName << std::endl;
	}

	const std::string str = oss.str();
	::write(stdoutfd, str.c_str(), str.size());
}

void EarlyExit()
{
	PrintMemCounters("early exit");
	PIN_Detach();
}

void RecordMemRead(RoutineRecord* routine, void* addr)
{
	AddrRecord& record = reads[ridx++];
	record.mRoutine = routine;
	record.mAddr = addr;
}

void RecordMemWrite(RoutineRecord* routine, void* addr)
{
	AddrRecord& record = writes[widx++];
	record.mRoutine = routine;
	record.mAddr = addr;
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

VOID CountInstructionPtr(UINT64 *counter)
{
	++(*counter);
}

ADDRINT CountDown()
{
	return insn == MaxInstructions || widx == MaxRecords || ridx == MaxRecords;
}

VOID Routine(RTN rtn, VOID*)
{
	routines.push_back(RoutineRecord{RTN_Name(rtn), RTN_Address(rtn)});
	RoutineRecord& record = routines.back();

	RTN_Open(rtn);
	RTN_InsertCall(rtn, IPOINT_BEFORE, (AFUNPTR)CountInstructionPtr, IARG_PTR, &(record.mCalls), IARG_END);

	for (INS ins = RTN_InsHead(rtn); INS_Valid(ins); ins = INS_Next(ins))
	{
		INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)CountInstructionPtr, IARG_PTR, &(record.mInstructions), IARG_END);

		UINT32 memOperands = INS_MemoryOperandCount(ins);

		for (UINT32 memOp = 0; memOp < memOperands; memOp++)
		{
			if (INS_MemoryOperandIsRead(ins, memOp))
			{
				INS_InsertIfCall(ins, IPOINT_BEFORE, (AFUNPTR)CountDown, IARG_END);
				INS_InsertThenCall(ins, IPOINT_BEFORE, (AFUNPTR)EarlyExit, IARG_END);

				INS_InsertPredicatedCall(
				    ins, IPOINT_BEFORE, (AFUNPTR)RecordMemRead,
				    IARG_PTR, &record,
				    IARG_MEMORYOP_EA, memOp,
				    IARG_END);
			}
			if (INS_MemoryOperandIsWritten(ins, memOp))
			{
				INS_InsertIfCall(ins, IPOINT_BEFORE, (AFUNPTR)CountDown, IARG_END);
				INS_InsertThenCall(ins, IPOINT_BEFORE, (AFUNPTR)EarlyExit, IARG_END);

				INS_InsertPredicatedCall(
				    ins, IPOINT_BEFORE, (AFUNPTR)RecordMemWrite,
				    IARG_PTR, &record,
				    IARG_MEMORYOP_EA, memOp,
				    IARG_END);
			}
		}
	}

	RTN_Close(rtn);
}

void Fini(INT32, void*)
{
	PrintMemCounters();
}

int main(int argc, char *argv[])
{
	if (PIN_Init(argc, argv))
	{
		PIN_ERROR("This Pintool evalutes the WSS (Working Set Size) of a process\n" + KNOB_BASE::StringKnobSummary() + "\n");
		return 1;
	}

	PIN_InitSymbols();

	stdoutfd = ::dup(1);
	reads = new AddrRecord[MaxRecords];
	writes = new AddrRecord[MaxRecords];

	TRACE_AddInstrumentFunction(Trace, 0);
	//INS_AddInstrumentFunction(Instruction, 0);
	RTN_AddInstrumentFunction(Routine, 0);
	PIN_AddFiniFunction(Fini, 0);

	PIN_StartProgram();
	return 0;
}

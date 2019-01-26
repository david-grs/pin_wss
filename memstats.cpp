#include "pin.H"

#include <cstdint>
#include <vector>
#include <algorithm>
#include <unordered_set>
#include <sstream>
#include <iostream>

extern "C"
{
#include <unistd.h>
}

struct MemCounters
{
	uint64_t mReads = 0;
	uint64_t mWrites = 0;
	std::tr1::unordered_set<void*> mUniqueReads;
	std::tr1::unordered_set<void*> mUniqueWrites;
	std::tr1::unordered_set<void*> mUniqueAccesses; // R+W
};

struct RoutineRecord
{
	RoutineRecord(const std::string& name, ADDRINT addr) :
	    mName(name), mAddr(addr)
	{}

	std::string mName;
	ADDRINT mAddr = 0;
	uint64_t mInstructions = 0;
	uint64_t mCalls = 0;

	MemCounters mCounters;
};

struct AddrRecord
{
	RoutineRecord* mRoutine = nullptr;
	void* mAddr = nullptr;
};

static const std::size_t MaxRecords = 256 * 1024 * 1024;
static const std::size_t MaxInstructions = 4000000000ULL;

std::vector<RoutineRecord*> routines; // Good old vector of pointers - Pin's STL Port is pre-C++11 so no move semantics...
AddrRecord *reads = nullptr;
AddrRecord *writes = nullptr;
uint64_t insn = 0;
uint64_t ridx = 0;
uint64_t widx = 0;
int stdoutfd;

template <typename T>
std::string to_string(T&& t)
{
	std::stringstream ss;
	ss << t;
	return ss.str();
}

std::string Format(std::size_t bytes, const std::string& unit = "")
{
	static const std::size_t Kibi = 1024;
	static const std::size_t Mibi = 1024 * 1024;
	static const std::size_t Gibi = 1024 * 1024 * 1024;

	if (bytes < 10 * Kibi)
		return to_string(bytes) + (unit.empty() ? "" : " ") + unit;
	else if (bytes < 10 * Mibi)
		return to_string(bytes / Kibi) + " Ki" + unit;
	else if (bytes < 10 * Gibi)
		return to_string(bytes / Mibi) + " Mi" + unit;
	return to_string(bytes / Mibi) + " Gi" + unit;
}

void PrintMemCounters(const char* reason = "")
{
	static const std::size_t CachelineBytes = 64;

	for (auto it = reads; it < reads + ridx; ++it)
	{
		MemCounters& c = it->mRoutine->mCounters;
		c.mReads += 1;
		c.mUniqueReads.insert((void*)(std::ptrdiff_t(it->mAddr) & (~(CachelineBytes - 1))));
	}
	for (auto it = writes; it < writes + widx; ++it)
	{
		MemCounters& c = it->mRoutine->mCounters;
		c.mWrites += 1;
		c.mUniqueWrites.insert((void*)(std::ptrdiff_t(it->mAddr) & (~(CachelineBytes - 1))));
	}

	for (auto& r : routines)
	{
		MemCounters& c = r->mCounters;
		c.mUniqueAccesses.insert(c.mUniqueReads.begin(), c.mUniqueReads.end());
		c.mUniqueAccesses.insert(c.mUniqueWrites.begin(), c.mUniqueWrites.end());
	}
	std::sort(routines.begin(), routines.end(), [](const auto& lhs, const auto& rhs)
	{
		return (lhs->mCounters.mReads + lhs->mCounters.mWrites) > (rhs->mCounters.mReads + rhs->mCounters.mWrites);
	});

	static const std::size_t Width = 13;
	std::ostringstream oss;
	oss << reason << std::endl
	    << std::setw(Width) << std::right << "reads"
	    << std::setw(Width) << std::right << "WSS (R)"
	    << std::setw(Width) << std::right << "writes"
	    << std::setw(Width) << std::right << "WSS (W)"
	    << std::setw(Width) << std::right << "WSS"
	    << std::setw(Width) << std::right << "calls"
	    << std::setw(Width) << std::right << "insn"
	    << std::setw(Width) << " "
	    << std::left << "function\n";

	for (const auto& r : routines)
	{
		const MemCounters& c = r->mCounters;
		oss << std::setw(Width) << std::right << Format(c.mReads * CachelineBytes)
		    << std::setw(Width) << std::right << Format(c.mUniqueReads.size() * CachelineBytes, "B")
		    << std::setw(Width) << std::right << Format(c.mWrites * CachelineBytes)
		    << std::setw(Width) << std::right << Format(c.mUniqueWrites.size() * CachelineBytes, "B")
		    << std::setw(Width) << std::right << Format(c.mUniqueAccesses.size() * CachelineBytes, "B")
		    << std::setw(Width) << std::right << r->mCalls
		    << std::setw(Width) << std::right << r->mInstructions
		    << std::setw(Width) << " "
		    << std::left << r->mName << std::endl;
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

VOID CountPtr(UINT64 *counter)
{
	++(*counter);
}

ADDRINT CountDown()
{
	return insn == MaxInstructions || widx == MaxRecords || ridx == MaxRecords;
}

VOID Routine(RTN rtn, VOID*)
{
	routines.push_back(new RoutineRecord{RTN_Name(rtn), RTN_Address(rtn)});
	RoutineRecord& record = *routines.back();

	RTN_Open(rtn);
	RTN_InsertCall(rtn, IPOINT_BEFORE, (AFUNPTR)CountPtr, IARG_PTR, &(record.mCalls), IARG_END);

	for (INS ins = RTN_InsHead(rtn); INS_Valid(ins); ins = INS_Next(ins))
	{
		INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)CountPtr, IARG_PTR, &(record.mInstructions), IARG_END);

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

	PIN_InitSymbolsAlt(SYMBOL_INFO_MODE(UINT32(IFUNC_SYMBOLS)));

	stdoutfd = ::dup(1);
	reads = new AddrRecord[MaxRecords];
	writes = new AddrRecord[MaxRecords];

	TRACE_AddInstrumentFunction(Trace, 0);
	RTN_AddInstrumentFunction(Routine, 0);
	PIN_AddFiniFunction(Fini, 0);

	PIN_StartProgram();
	return 0;
}

#pragma once

#include "pin.H"

#include <cstdint>
#include <vector>
#include <unordered_set>

namespace WSS
{

static const std::size_t sMaxRecords = 256 * 1024 * 1024;
static const std::size_t sMaxInstructions = 4000000000ULL;

struct MemCounters
{
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

void PrintWSS(const char* reason);

}

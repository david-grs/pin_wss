#include "wss_analysis.h"

#include <algorithm>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <iostream>

namespace WSS { namespace Analysis {

static const std::size_t sCachelineBytes = 64;

std::vector<RoutineRecord*> sRoutines; // good old vector of pointers - Pin's STL Port is pre-C++11, so no move semantics...
RoutineRecord* sCurrRoutine = nullptr;
AddrRecord *sReads = nullptr;
AddrRecord *sWrites = nullptr;
uint64_t sInsn = 0;
uint64_t sRidx = 0;
uint64_t sWidx = 0;
std::ofstream sOutputFile;

void Init(const std::string& filename)
{
	sReads = new WSS::AddrRecord[sMaxRecords];
	sWrites = new WSS::AddrRecord[sMaxRecords];

	sOutputFile.open(filename.c_str());
}

void Fini()
{
	for (auto it = sReads; it < sReads + sRidx; ++it)
	{
		MemCounters& c = it->mRoutine->mCounters;
		c.mUniqueReads.insert((void*)(std::ptrdiff_t(it->mAddr) & (~(sCachelineBytes - 1))));
	}

	for (auto it = sWrites; it < sWrites + sWidx; ++it)
	{
		MemCounters& c = it->mRoutine->mCounters;
		c.mUniqueWrites.insert((void*)(std::ptrdiff_t(it->mAddr) & (~(sCachelineBytes - 1))));
	}

	for (auto& r : sRoutines)
	{
		MemCounters& c = r->mCounters;
		c.mUniqueAccesses.insert(c.mUniqueReads.begin(), c.mUniqueReads.end());
		c.mUniqueAccesses.insert(c.mUniqueWrites.begin(), c.mUniqueWrites.end());
	}
}

void EarlyExit()
{
	PIN_Detach();

	std::cout << "Pin: early exit" << std::endl;
	Fini();
	PrintWSS();
}

template <typename T>
std::string to_string(const T& t)
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
	return to_string(bytes / Gibi) + " Gi" + unit;
}

void PrintWSS()
{
	std::sort(sRoutines.begin(), sRoutines.end(), [](const auto& lhs, const auto& rhs)
	{
		return lhs->mCounters.mUniqueAccesses.size() > rhs->mCounters.mUniqueAccesses.size();
	});

	static const std::size_t Width = 13;
	std::ostringstream oss;
	oss << std::setw(Width) << std::right << "WSS (R)"
	    << std::setw(Width) << std::right << "WSS (W)"
	    << std::setw(Width) << std::right << "WSS"
	    << std::setw(Width) << std::right << "calls"
	    << std::setw(Width) << std::right << "insn"
	    << std::setw(6) << " "
	    << std::left << "function\n";

	for (const auto& r : sRoutines)
	{
		const MemCounters& c = r->mCounters;
		oss << std::setw(Width) << std::right << Format(c.mUniqueReads.size() * sCachelineBytes, "B")
		    << std::setw(Width) << std::right << Format(c.mUniqueWrites.size() * sCachelineBytes, "B")
		    << std::setw(Width) << std::right << Format(c.mUniqueAccesses.size() * sCachelineBytes, "B")
		    << std::setw(Width) << std::right << r->mCalls
		    << std::setw(Width) << std::right << r->mInstructions
		    << std::setw(6) << " "
		    << std::left << r->mName << std::endl;
	}

	sOutputFile << oss.str();
	sOutputFile.close();
}

}}

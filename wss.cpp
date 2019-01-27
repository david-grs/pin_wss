#include "wss.h"
#include "wss_instr.h"
#include "wss_analysis.h"

#include "pin.H"

#include <algorithm>
#include <sstream>
#include <fstream>
#include <iostream>

KNOB<std::string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool", "o", "wss.out", "output file");

int main(int argc, char *argv[])
{
	if (PIN_Init(argc, argv))
	{
		PIN_ERROR("This Pintool evalutes the WSS (Working Set Size) of a process\n" + KNOB_BASE::StringKnobSummary() + "\n");
		return 1;
	}

	PIN_InitSymbols();

	WSS::Analysis::Init(KnobOutputFile.Value());
	WSS::Instrumentation::Init();

	PIN_StartProgram();

	return 0;
}

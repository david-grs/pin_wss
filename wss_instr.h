#pragma once

#include "pin.H"

namespace WSS { namespace Instrumentation {

void Init();

void Trace(TRACE, void*);
void Instruction(INS, void*);
void Routine(RTN, void*);
void Fini(INT32, void*);

}}

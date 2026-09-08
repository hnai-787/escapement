#ifndef CPU16_REPORT_HPP
#define CPU16_REPORT_HPP

#include <string>

#include "cpu16/cpu.hpp"

namespace cpu16 {

// "instruction" level: one line per instruction (PC, disassembly, register changes).
std::string renderInstructionSummary(const InstructionTrace& trace);
// "cycle" level: every phase of the instruction with its bus transaction, if any.
std::string renderCycleDetail(const InstructionTrace& trace);

std::string renderRegisters(const Registers& r);
std::string renderFlags(const Flags& f);

}  // namespace cpu16

#endif

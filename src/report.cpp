#include "cpu16/report.hpp"

#include <iomanip>
#include <sstream>

namespace cpu16 {

namespace {
std::string hex(std::uint16_t v, int width = 3) {
    std::ostringstream oss;
    oss << "0x" << std::hex << std::setw(width) << std::setfill('0') << v;
    return oss.str();
}
}  // namespace

std::string renderRegisters(const Registers& r) {
    std::ostringstream oss;
    oss << "R1=" << r.r1 << " R2=" << r.r2 << " R3=" << r.r3 << " RES=" << r.res;
    return oss.str();
}

std::string renderFlags(const Flags& f) {
    std::ostringstream oss;
    oss << "Z=" << (f.zero ? 1 : 0) << " N=" << (f.negative ? 1 : 0);
    return oss.str();
}

std::string renderInstructionSummary(const InstructionTrace& t) {
    std::ostringstream oss;
    oss << "PC=" << hex(t.pcBefore) << "  " << hex(t.instructionWord, 4) << "  " << t.disassembly;
    if (t.faulted) {
        oss << "  -- FAULT: " << faultName(t.fault);
    } else if (t.halted) {
        oss << "  -- HALT";
    }
    oss << "\n    " << renderRegisters(t.registersAfter) << "  " << renderFlags(t.flagsAfter);
    return oss.str();
}

std::string renderCycleDetail(const InstructionTrace& t) {
    std::ostringstream oss;
    oss << "PC=" << hex(t.pcBefore) << "  " << hex(t.instructionWord, 4) << "  " << t.disassembly << "\n";
    for (const PhaseRecord& p : t.phases) {
        oss << "  " << phaseName(p.phase);
        if (p.hasBus) {
            const ControlSignals& c = p.bus.control;
            oss << "  AddrBus=" << hex(p.bus.address) << " DataBus=" << hex(p.bus.data, 4) << " ["
                << (c.read ? "READ " : "") << (c.write ? "WRITE " : "") << (c.romSelect ? "ROM_SEL " : "")
                << (c.ramSelect ? "RAM_SEL " : "") << (c.ioSelect ? "IO_SEL " : "") << "]";
        }
        oss << "  " << p.note << "\n";
    }
    if (t.faulted) oss << "  FAULT: " << faultName(t.fault) << "\n";
    if (t.halted) oss << "  HALT\n";
    oss << "  " << renderRegisters(t.registersAfter) << "  " << renderFlags(t.flagsAfter) << "\n";
    return oss.str();
}

}  // namespace cpu16

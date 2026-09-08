#ifndef CPU16_CPU_HPP
#define CPU16_CPU_HPP

#include <cstdint>
#include <deque>
#include <string>
#include <vector>

#include "cpu16/isa.hpp"

namespace cpu16 {

// Memory map for the 10-bit (1024-word) address space -- see README
// "Design decisions" for why this is a *shared* address/data/control bus
// with address-decoded device selection, not a true Harvard architecture,
// even though the original diagram draws ROM and RAM as separate boxes.
constexpr std::uint16_t kRomStart = 0x000;
constexpr std::uint16_t kRomEnd = 0x1FF;    // inclusive
constexpr std::uint16_t kRamStart = 0x200;
constexpr std::uint16_t kRamEnd = 0x3EF;    // inclusive
constexpr std::uint16_t kMmioStart = 0x3F0;
constexpr std::uint16_t kMmioEnd = 0x3FF;   // inclusive
constexpr std::uint16_t kAddrInputData = 0x3F0;
constexpr std::uint16_t kAddrOutputData = 0x3F1;
constexpr std::uint16_t kAddressMask = 0x3FF;

enum class CpuFault { None, WriteToRom, DivideByZero, InvalidDeviceAccess };

std::string faultName(CpuFault f);

struct Registers {
    std::int16_t r1 = 0, r2 = 0, r3 = 0, res = 0;

    std::int16_t get(Reg r) const;
    void set(Reg r, std::int16_t value);
};

struct Flags {
    bool zero = false;
    bool negative = false;
};

// The address/data/control bus state for one phase of one instruction --
// this is what makes the original BusArchitecture diagram's three buses
// concretely observable, not just implied. `romSelect`/`ramSelect`/
// `ioSelect` are mutually exclusive by construction (see the bus-exclusivity
// test in tests/test_cpu.cpp).
struct ControlSignals {
    bool read = false;
    bool write = false;
    bool romSelect = false;
    bool ramSelect = false;
    bool ioSelect = false;
};

struct BusTransaction {
    std::uint16_t address = 0;
    std::uint16_t data = 0;
    ControlSignals control;
};

enum class CpuPhase { Fetch, Decode, Execute, Memory, WriteBack };

std::string phaseName(CpuPhase p);

struct PhaseRecord {
    CpuPhase phase;
    bool hasBus = false;
    BusTransaction bus;
    std::string note;  // short human-readable description of what happened
};

struct InstructionTrace {
    std::uint16_t pcBefore = 0;
    std::uint16_t instructionWord = 0;
    std::string disassembly;
    std::vector<PhaseRecord> phases;
    Registers registersBefore, registersAfter;
    Flags flagsBefore, flagsAfter;
    bool halted = false;
    bool faulted = false;
    CpuFault fault = CpuFault::None;
};

class Cpu {
public:
    // `rom` must fit within [kRomStart, kRomEnd]. `inputQueue` seeds the
    // memory-mapped input device (each read of kAddrInputData pops the
    // next value; reading past the end returns 0 rather than faulting --
    // see README "Limitations").
    explicit Cpu(std::vector<std::uint16_t> rom, std::vector<std::int16_t> inputQueue = {});

    // Executes exactly one full instruction (fetch through write-back) and
    // returns a trace with a PhaseRecord per micro-phase, each carrying
    // its own bus transaction where relevant. This is a deliberate
    // simplification of true resumable microcycle stepping (which would
    // need the CPU to persist mid-instruction FSM state across separate
    // calls) -- the same bus-level detail is still fully visible in the
    // returned trace, just computed in one call. See README "Design
    // decisions".
    InstructionTrace step();

    bool halted() const { return halted_; }
    bool faulted() const { return fault_ != CpuFault::None; }
    CpuFault fault() const { return fault_; }
    const Registers& registers() const { return registers_; }
    const Flags& flags() const { return flags_; }
    std::uint16_t pc() const { return pc_; }
    const std::vector<std::int16_t>& outputLog() const { return outputLog_; }

private:
    std::vector<std::uint16_t> rom_;
    std::vector<std::uint16_t> ram_;
    std::deque<std::int16_t> inputQueue_;
    std::vector<std::int16_t> outputLog_;

    Registers registers_;
    Flags flags_;
    std::uint16_t pc_ = 0;
    bool halted_ = false;
    CpuFault fault_ = CpuFault::None;

    std::uint16_t readBus(std::uint16_t address, PhaseRecord& record);
    void writeBus(std::uint16_t address, std::uint16_t data, PhaseRecord& record);
    void updateFlags(std::int16_t result);
};

}  // namespace cpu16

#endif

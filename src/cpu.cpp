#include "cpu16/cpu.hpp"

#include <sstream>
#include <stdexcept>

namespace cpu16 {

namespace {
std::string toHex(std::uint16_t v) {
    std::ostringstream oss;
    oss << std::hex << v;
    return oss.str();
}
}  // namespace

std::string faultName(CpuFault f) {
    switch (f) {
        case CpuFault::None: return "None";
        case CpuFault::WriteToRom: return "WriteToRom";
        case CpuFault::DivideByZero: return "DivideByZero";
        case CpuFault::InvalidDeviceAccess: return "InvalidDeviceAccess";
    }
    return "Unknown";
}

std::string phaseName(CpuPhase p) {
    switch (p) {
        case CpuPhase::Fetch: return "FETCH";
        case CpuPhase::Decode: return "DECODE";
        case CpuPhase::Execute: return "EXECUTE";
        case CpuPhase::Memory: return "MEMORY";
        case CpuPhase::WriteBack: return "WRITEBACK";
    }
    return "?";
}

std::int16_t Registers::get(Reg r) const {
    switch (r) {
        case Reg::R1: return r1;
        case Reg::R2: return r2;
        case Reg::R3: return r3;
        case Reg::RES: return res;
    }
    return 0;
}

void Registers::set(Reg r, std::int16_t value) {
    switch (r) {
        case Reg::R1: r1 = value; break;
        case Reg::R2: r2 = value; break;
        case Reg::R3: r3 = value; break;
        case Reg::RES: res = value; break;
    }
}

Cpu::Cpu(std::vector<std::uint16_t> rom, std::vector<std::int16_t> inputQueue)
    : rom_(std::move(rom)), ram_(kRamEnd - kRamStart + 1, 0), inputQueue_(inputQueue.begin(), inputQueue.end()) {
    if (rom_.size() > static_cast<std::size_t>(kRomEnd - kRomStart + 1)) {
        throw std::invalid_argument("Cpu: ROM image too large for the 0x000-0x1FF program region");
    }
    rom_.resize(kRomEnd - kRomStart + 1, 0);  // pad with NOP (0x0000)
}

std::uint16_t Cpu::readBus(std::uint16_t address, PhaseRecord& record) {
    address &= kAddressMask;
    record.hasBus = true;
    record.bus.address = address;
    record.bus.control.read = true;

    std::uint16_t data = 0;
    if (address >= kRomStart && address <= kRomEnd) {
        record.bus.control.romSelect = true;
        data = rom_[address - kRomStart];
    } else if (address >= kRamStart && address <= kRamEnd) {
        record.bus.control.ramSelect = true;
        data = ram_[address - kRamStart];
    } else {
        record.bus.control.ioSelect = true;
        if (address == kAddrInputData) {
            if (!inputQueue_.empty()) {
                data = static_cast<std::uint16_t>(inputQueue_.front());
                inputQueue_.pop_front();
            }
        } else {
            fault_ = CpuFault::InvalidDeviceAccess;
        }
    }
    record.bus.data = data;
    return data;
}

void Cpu::writeBus(std::uint16_t address, std::uint16_t data, PhaseRecord& record) {
    address &= kAddressMask;
    record.hasBus = true;
    record.bus.address = address;
    record.bus.data = data;
    record.bus.control.write = true;

    if (address >= kRomStart && address <= kRomEnd) {
        record.bus.control.romSelect = true;
        fault_ = CpuFault::WriteToRom;
    } else if (address >= kRamStart && address <= kRamEnd) {
        record.bus.control.ramSelect = true;
        ram_[address - kRamStart] = data;
    } else {
        record.bus.control.ioSelect = true;
        if (address == kAddrOutputData) {
            outputLog_.push_back(static_cast<std::int16_t>(data));
        } else {
            fault_ = CpuFault::InvalidDeviceAccess;
        }
    }
}

void Cpu::updateFlags(std::int16_t result) {
    flags_.zero = (result == 0);
    flags_.negative = (result < 0);
}

InstructionTrace Cpu::step() {
    InstructionTrace trace;
    trace.registersBefore = registers_;
    trace.flagsBefore = flags_;
    trace.pcBefore = pc_;

    if (halted_ || fault_ != CpuFault::None) {
        trace.halted = halted_;
        trace.faulted = fault_ != CpuFault::None;
        trace.fault = fault_;
        trace.registersAfter = registers_;
        trace.flagsAfter = flags_;
        return trace;
    }

    // FETCH
    PhaseRecord fetchRec;
    fetchRec.phase = CpuPhase::Fetch;
    std::uint16_t word = readBus(pc_, fetchRec);
    fetchRec.note = "IR <- Mem[0x" + toHex(pc_) + "] = 0x" + toHex(word);
    trace.phases.push_back(fetchRec);
    trace.instructionWord = word;
    pc_ = static_cast<std::uint16_t>((pc_ + 1) & kAddressMask);

    if (fault_ != CpuFault::None) {
        trace.faulted = true;
        trace.fault = fault_;
        trace.registersAfter = registers_;
        trace.flagsAfter = flags_;
        return trace;
    }

    // DECODE
    DecodedInstruction instr = decode(word);
    trace.disassembly = disassemble(instr);
    PhaseRecord decodeRec;
    decodeRec.phase = CpuPhase::Decode;
    decodeRec.note = "decoded: " + trace.disassembly;
    trace.phases.push_back(decodeRec);

    // EXECUTE (+ MEMORY / WRITEBACK where applicable)
    switch (instr.opcode) {
        case Opcode::NOP:
            break;

        case Opcode::HALT:
            halted_ = true;
            break;

        case Opcode::LDI: {
            Reg dst = static_cast<Reg>(instr.field1);
            registers_.set(dst, instr.imm);
            updateFlags(instr.imm);
            PhaseRecord rec;
            rec.phase = CpuPhase::Execute;
            rec.note = regName(dst) + " <- " + std::to_string(instr.imm);
            trace.phases.push_back(rec);
            break;
        }

        case Opcode::LD: {
            Reg dst = static_cast<Reg>(instr.field1);
            PhaseRecord memRec;
            memRec.phase = CpuPhase::Memory;
            std::uint16_t value = readBus(static_cast<std::uint16_t>(instr.imm), memRec);
            memRec.note = "MDR <- Mem[0x" + toHex(static_cast<std::uint16_t>(instr.imm)) + "]";
            trace.phases.push_back(memRec);
            if (fault_ == CpuFault::None) {
                registers_.set(dst, static_cast<std::int16_t>(value));
                updateFlags(static_cast<std::int16_t>(value));
                PhaseRecord wbRec;
                wbRec.phase = CpuPhase::WriteBack;
                wbRec.note = regName(dst) + " <- 0x" + toHex(value);
                trace.phases.push_back(wbRec);
            }
            break;
        }

        case Opcode::ST: {
            Reg src = static_cast<Reg>(instr.field1);
            PhaseRecord memRec;
            memRec.phase = CpuPhase::Memory;
            writeBus(static_cast<std::uint16_t>(instr.imm), static_cast<std::uint16_t>(registers_.get(src)), memRec);
            memRec.note = "Mem[0x" + toHex(static_cast<std::uint16_t>(instr.imm)) + "] <- " + regName(src);
            trace.phases.push_back(memRec);
            break;
        }

        case Opcode::BR: {
            BranchCond cond = static_cast<BranchCond>(instr.field1);
            bool take = false;
            switch (cond) {
                case BranchCond::Always: take = true; break;
                case BranchCond::Zero: take = flags_.zero; break;
                case BranchCond::NotZero: take = !flags_.zero; break;
                case BranchCond::Negative: take = flags_.negative; break;
            }
            PhaseRecord rec;
            rec.phase = CpuPhase::Execute;
            if (take) {
                pc_ = static_cast<std::uint16_t>((pc_ + instr.imm) & kAddressMask);
                rec.note = "branch taken -> PC=0x" + toHex(pc_);
            } else {
                rec.note = "branch not taken";
            }
            trace.phases.push_back(rec);
            break;
        }

        default: {
            // Register-register format: MOV, ADD, SUB, MUL, DIV, AND, OR, XOR, CMP, NOT
            Reg dst = static_cast<Reg>(instr.field1);
            Reg src = static_cast<Reg>(instr.field2);
            std::int16_t a = registers_.get(dst);
            std::int16_t b = registers_.get(src);
            std::int16_t result = 0;
            bool writesDst = true;

            switch (instr.opcode) {
                case Opcode::MOV: result = b; break;
                case Opcode::ADD: result = static_cast<std::int16_t>(a + b); break;
                case Opcode::SUB: result = static_cast<std::int16_t>(a - b); break;
                case Opcode::MUL: result = static_cast<std::int16_t>(a * b); break;
                case Opcode::DIV:
                    if (b == 0) {
                        fault_ = CpuFault::DivideByZero;
                        writesDst = false;
                    } else {
                        result = static_cast<std::int16_t>(a / b);
                    }
                    break;
                case Opcode::AND: result = static_cast<std::int16_t>(a & b); break;
                case Opcode::OR: result = static_cast<std::int16_t>(a | b); break;
                case Opcode::XOR: result = static_cast<std::int16_t>(a ^ b); break;
                case Opcode::CMP:
                    result = static_cast<std::int16_t>(a - b);
                    writesDst = false;
                    break;
                case Opcode::NOT: result = static_cast<std::int16_t>(~b); break;
                default: break;
            }

            PhaseRecord rec;
            rec.phase = CpuPhase::Execute;
            if (fault_ == CpuFault::None) {
                updateFlags(result);
                if (writesDst) {
                    registers_.set(dst, result);
                    rec.note = regName(dst) + " <- 0x" + toHex(static_cast<std::uint16_t>(result));
                } else {
                    rec.note = "flags updated (result=" + std::to_string(result) + ")";
                }
            } else {
                rec.note = "DIVIDE BY ZERO";
            }
            trace.phases.push_back(rec);
            break;
        }
    }

    trace.halted = halted_;
    trace.faulted = fault_ != CpuFault::None;
    trace.fault = fault_;
    trace.registersAfter = registers_;
    trace.flagsAfter = flags_;
    return trace;
}

}  // namespace cpu16

#include "cpu16/isa.hpp"

#include <array>
#include <sstream>

namespace cpu16 {

namespace {
constexpr std::uint16_t k10BitMask = 0x03FF;
}

std::string regName(Reg r) {
    switch (r) {
        case Reg::R1: return "R1";
        case Reg::R2: return "R2";
        case Reg::R3: return "R3";
        case Reg::RES: return "RES";
    }
    return "?";
}

std::optional<Reg> regFromName(const std::string& name) {
    if (name == "R1") return Reg::R1;
    if (name == "R2") return Reg::R2;
    if (name == "R3") return Reg::R3;
    if (name == "RES") return Reg::RES;
    return std::nullopt;
}

std::string opcodeName(Opcode op) {
    switch (op) {
        case Opcode::NOP: return "NOP";
        case Opcode::LDI: return "LDI";
        case Opcode::LD: return "LD";
        case Opcode::ST: return "ST";
        case Opcode::MOV: return "MOV";
        case Opcode::ADD: return "ADD";
        case Opcode::SUB: return "SUB";
        case Opcode::MUL: return "MUL";
        case Opcode::DIV: return "DIV";
        case Opcode::AND: return "AND";
        case Opcode::OR: return "OR";
        case Opcode::XOR: return "XOR";
        case Opcode::CMP: return "CMP";
        case Opcode::BR: return "BR";
        case Opcode::NOT: return "NOT";
        case Opcode::HALT: return "HALT";
    }
    return "?";
}

std::optional<Opcode> opcodeFromName(const std::string& name) {
    static const std::array<Opcode, 16> all = {
        Opcode::NOP, Opcode::LDI, Opcode::LD,  Opcode::ST,  Opcode::MOV, Opcode::ADD,
        Opcode::SUB, Opcode::MUL, Opcode::DIV, Opcode::AND, Opcode::OR,  Opcode::XOR,
        Opcode::CMP, Opcode::BR,  Opcode::NOT, Opcode::HALT,
    };
    for (Opcode op : all) {
        if (opcodeName(op) == name) return op;
    }
    return std::nullopt;
}

bool isRegRegFormat(Opcode op) {
    switch (op) {
        case Opcode::MOV:
        case Opcode::ADD:
        case Opcode::SUB:
        case Opcode::MUL:
        case Opcode::DIV:
        case Opcode::AND:
        case Opcode::OR:
        case Opcode::XOR:
        case Opcode::CMP:
        case Opcode::NOT:
            return true;
        default:
            return false;
    }
}

bool isRegImmFormat(Opcode op) { return op == Opcode::LDI || op == Opcode::LD || op == Opcode::ST; }
bool isBranchFormat(Opcode op) { return op == Opcode::BR; }
bool isNoOperandFormat(Opcode op) { return op == Opcode::NOP || op == Opcode::HALT; }

std::int16_t signExtend10(std::uint16_t value10) {
    value10 &= k10BitMask;
    if (value10 & 0x0200) {  // sign bit of a 10-bit field
        return static_cast<std::int16_t>(value10 | 0xFC00);
    }
    return static_cast<std::int16_t>(value10);
}

std::uint16_t truncate10(std::int16_t value) { return static_cast<std::uint16_t>(value) & k10BitMask; }

std::uint16_t encodeRegReg(Opcode op, Reg dst, Reg src) {
    return static_cast<std::uint16_t>((static_cast<std::uint16_t>(op) << 12) |
                                       (static_cast<std::uint16_t>(dst) << 10) |
                                       (static_cast<std::uint16_t>(src) << 8));
}

std::uint16_t encodeRegImm(Opcode op, Reg reg, std::int16_t imm10) {
    return static_cast<std::uint16_t>((static_cast<std::uint16_t>(op) << 12) |
                                       (static_cast<std::uint16_t>(reg) << 10) | truncate10(imm10));
}

std::uint16_t encodeBranch(BranchCond cond, std::int16_t offset10) {
    return static_cast<std::uint16_t>((static_cast<std::uint16_t>(Opcode::BR) << 12) |
                                       (static_cast<std::uint16_t>(cond) << 10) | truncate10(offset10));
}

std::uint16_t encodeNoOperand(Opcode op) {
    return static_cast<std::uint16_t>(static_cast<std::uint16_t>(op) << 12);
}

DecodedInstruction decode(std::uint16_t word) {
    DecodedInstruction d;
    d.raw = word;
    d.opcode = static_cast<Opcode>((word >> 12) & 0xF);

    if (isRegRegFormat(d.opcode)) {
        d.field1 = (word >> 10) & 0x3;
        d.field2 = (word >> 8) & 0x3;
    } else if (d.opcode == Opcode::LDI) {
        d.field1 = (word >> 10) & 0x3;
        d.imm = signExtend10(word & k10BitMask);
    } else if (d.opcode == Opcode::LD || d.opcode == Opcode::ST) {
        d.field1 = (word >> 10) & 0x3;
        d.imm = static_cast<std::int16_t>(word & k10BitMask);  // raw unsigned address
    } else if (isBranchFormat(d.opcode)) {
        d.field1 = (word >> 10) & 0x3;
        d.imm = signExtend10(word & k10BitMask);
    }
    return d;
}

std::string disassemble(const DecodedInstruction& d) {
    std::ostringstream oss;
    Reg dst = static_cast<Reg>(d.field1);
    Reg src = static_cast<Reg>(d.field2);

    switch (d.opcode) {
        case Opcode::NOP: return "NOP";
        case Opcode::HALT: return "HALT";
        case Opcode::LDI:
            oss << "LDI " << regName(dst) << ", " << d.imm;
            return oss.str();
        case Opcode::LD:
            oss << "LD " << regName(dst) << ", [0x" << std::hex << d.imm << "]";
            return oss.str();
        case Opcode::ST:
            oss << "ST " << regName(dst) << ", [0x" << std::hex << d.imm << "]";
            return oss.str();
        case Opcode::BR: {
            const char* mnemonic = "BR";
            switch (static_cast<BranchCond>(d.field1)) {
                case BranchCond::Always: mnemonic = "BRA"; break;
                case BranchCond::Zero: mnemonic = "BZ"; break;
                case BranchCond::NotZero: mnemonic = "BNZ"; break;
                case BranchCond::Negative: mnemonic = "BN"; break;
            }
            oss << mnemonic << " " << (d.imm >= 0 ? "+" : "") << d.imm;
            return oss.str();
        }
        default:
            oss << opcodeName(d.opcode) << " " << regName(dst) << ", " << regName(src);
            return oss.str();
    }
}

std::string disassemble(std::uint16_t word) { return disassemble(decode(word)); }

}  // namespace cpu16

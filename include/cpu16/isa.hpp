#ifndef CPU16_ISA_HPP
#define CPU16_ISA_HPP

#include <cstdint>
#include <optional>
#include <string>

namespace cpu16 {

// The instruction set implementing this project's original CPU_BlockDiagram
// and BusArchitecture diagrams: a 16-bit fixed-width machine word, a 4-bit
// opcode (all 16 values assigned -- there is no "illegal opcode" possible
// by construction, unlike some teaching ISAs that reserve slots), and a
// 4-register file (R1, R2, R3, RES) matching the diagram's Register Array
// exactly. See README "Design decisions" for the full rationale and the
// citation for this being in the SAP-1/LC-3 pedagogical tradition.
enum class Opcode : std::uint8_t {
    NOP = 0x0,
    LDI = 0x1,
    LD = 0x2,
    ST = 0x3,
    MOV = 0x4,
    ADD = 0x5,
    SUB = 0x6,
    MUL = 0x7,
    DIV = 0x8,
    AND = 0x9,
    OR = 0xA,
    XOR = 0xB,
    CMP = 0xC,
    BR = 0xD,
    NOT = 0xE,
    HALT = 0xF,
};

// Registers are 2-bit encoded exactly as the diagram names them (not
// renumbered to a zero-based R0..R3 scheme, to keep the mapping to the
// original Register Array box obvious).
enum class Reg : std::uint8_t { R1 = 0, R2 = 1, R3 = 2, RES = 3 };

std::string regName(Reg r);
std::optional<Reg> regFromName(const std::string& name);

// Branch condition codes (packed into the same 2-bit field a register
// index would occupy in the register-register format).
enum class BranchCond : std::uint8_t { Always = 0, Zero = 1, NotZero = 2, Negative = 3 };

std::string opcodeName(Opcode op);
std::optional<Opcode> opcodeFromName(const std::string& name);

// True for ADD/SUB/MUL/DIV/AND/OR/XOR/CMP/MOV/NOT (register-register
// format: opcode(4) | dst(2) | src(2) | reserved(8)).
bool isRegRegFormat(Opcode op);
// True for LDI/LD/ST (register-immediate/address format: opcode(4) |
// reg(2) | imm10/addr10(10)).
bool isRegImmFormat(Opcode op);
// True for BR (opcode(4) | cond(2) | offset10(10), signed, PC-relative).
bool isBranchFormat(Opcode op);
// True for NOP/HALT (opcode(4) | reserved(12)).
bool isNoOperandFormat(Opcode op);

std::uint16_t encodeRegReg(Opcode op, Reg dst, Reg src);
std::uint16_t encodeRegImm(Opcode op, Reg reg, std::int16_t imm10);
std::uint16_t encodeBranch(BranchCond cond, std::int16_t offset10);
std::uint16_t encodeNoOperand(Opcode op);

struct DecodedInstruction {
    Opcode opcode;
    std::uint8_t field1 = 0;  // dst register, or reg (LD/ST), or branch cond
    std::uint8_t field2 = 0;  // src register (reg-reg format only)
    std::int16_t imm = 0;     // sign-extended imm10/offset10, or raw addr10
    std::uint16_t raw = 0;
};

DecodedInstruction decode(std::uint16_t word);
std::string disassemble(const DecodedInstruction& instr);
std::string disassemble(std::uint16_t word);

// Sign-extends a 10-bit two's-complement value to a full 16-bit int16_t.
std::int16_t signExtend10(std::uint16_t value10);
// Masks to the 10-bit unsigned range actually representable in the
// imm10/addr10 field (matches how the real hardware would truncate it).
std::uint16_t truncate10(std::int16_t value);

}  // namespace cpu16

#endif

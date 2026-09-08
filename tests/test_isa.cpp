#include <catch2/catch_test_macros.hpp>

#include "cpu16/isa.hpp"

using namespace cpu16;

TEST_CASE("ADD R1, R2 encodes to the exact worked example (0x5100)", "[isa]") {
    // Matches this project's research derivation exactly: opcode ADD=0101,
    // dst=R1=00, src=R2=01, reserved=00000000 -> 0101 0001 0000 0000.
    REQUIRE(encodeRegReg(Opcode::ADD, Reg::R1, Reg::R2) == 0x5100);
}

TEST_CASE("register-register round trip for every ALU/logic opcode", "[isa]") {
    for (Opcode op : {Opcode::MOV, Opcode::ADD, Opcode::SUB, Opcode::MUL, Opcode::DIV, Opcode::AND, Opcode::OR,
                       Opcode::XOR, Opcode::CMP, Opcode::NOT}) {
        std::uint16_t word = encodeRegReg(op, Reg::R3, Reg::RES);
        DecodedInstruction d = decode(word);
        REQUIRE(d.opcode == op);
        REQUIRE(static_cast<Reg>(d.field1) == Reg::R3);
        REQUIRE(static_cast<Reg>(d.field2) == Reg::RES);
    }
}

TEST_CASE("LDI round trip preserves sign", "[isa]") {
    std::uint16_t word = encodeRegImm(Opcode::LDI, Reg::R2, -42);
    DecodedInstruction d = decode(word);
    REQUIRE(d.opcode == Opcode::LDI);
    REQUIRE(static_cast<Reg>(d.field1) == Reg::R2);
    REQUIRE(d.imm == -42);
}

TEST_CASE("LD/ST round trip preserves an unsigned 10-bit address", "[isa]") {
    std::uint16_t word = encodeRegImm(Opcode::LD, Reg::R1, 0x220);
    DecodedInstruction d = decode(word);
    REQUIRE(d.opcode == Opcode::LD);
    REQUIRE(d.imm == 0x220);
}

TEST_CASE("branch round trip preserves a negative PC-relative offset", "[isa]") {
    std::uint16_t word = encodeBranch(BranchCond::NotZero, -5);
    DecodedInstruction d = decode(word);
    REQUIRE(d.opcode == Opcode::BR);
    REQUIRE(static_cast<BranchCond>(d.field1) == BranchCond::NotZero);
    REQUIRE(d.imm == -5);
}

TEST_CASE("sign extension of a 10-bit field", "[isa]") {
    REQUIRE(signExtend10(0x000) == 0);
    REQUIRE(signExtend10(0x001) == 1);
    REQUIRE(signExtend10(0x1FF) == 511);   // largest positive 10-bit value
    REQUIRE(signExtend10(0x200) == -512);  // smallest negative (sign bit set)
    REQUIRE(signExtend10(0x3FF) == -1);
}

TEST_CASE("truncate10 wraps to the 10-bit field exactly", "[isa]") {
    REQUIRE(truncate10(-1) == 0x3FF);
    REQUIRE(truncate10(-512) == 0x200);
    REQUIRE(truncate10(511) == 0x1FF);
}

TEST_CASE("disassembly is human-readable for every instruction shape", "[isa]") {
    REQUIRE(disassemble(encodeRegReg(Opcode::ADD, Reg::R1, Reg::R2)) == "ADD R1, R2");
    REQUIRE(disassemble(encodeRegImm(Opcode::LDI, Reg::R1, 5)) == "LDI R1, 5");
    REQUIRE(disassemble(encodeNoOperand(Opcode::HALT)) == "HALT");
    REQUIRE(disassemble(encodeBranch(BranchCond::Zero, 3)) == "BZ +3");
}

TEST_CASE("the full 4-bit opcode space is assigned (no illegal-opcode case exists by construction)", "[isa]") {
    for (int i = 0; i < 16; ++i) {
        REQUIRE(opcodeName(static_cast<Opcode>(i)) != "?");
    }
}

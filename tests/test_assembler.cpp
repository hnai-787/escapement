#include <catch2/catch_test_macros.hpp>

#include "cpu16/assembler.hpp"
#include "cpu16/cpu.hpp"

using namespace cpu16;

TEST_CASE("assembles a minimal program", "[assembler]") {
    AssembleResult r = assemble("LDI R1, 5\nHALT\n");
    REQUIRE(r.ok);
    REQUIRE(r.rom.size() == 2);
    REQUIRE(r.rom[0] == encodeRegImm(Opcode::LDI, Reg::R1, 5));
    REQUIRE(r.rom[1] == encodeNoOperand(Opcode::HALT));
}

TEST_CASE("labels resolve to the correct address, including forward references", "[assembler]") {
    std::string src =
        "    BRA SKIP\n"
        "    LDI R1, 99\n"
        "SKIP:\n"
        "    LDI R1, 1\n"
        "    HALT\n";
    AssembleResult r = assemble(src);
    REQUIRE(r.ok);
    REQUIRE(r.labels.at("SKIP") == 2);
}

TEST_CASE("IN/OUT pseudo-instructions expand to the documented LD/ST forms", "[assembler]") {
    AssembleResult r = assemble("IN R1\nOUT R1\n");
    REQUIRE(r.ok);
    REQUIRE(r.rom[0] == encodeRegImm(Opcode::LD, Reg::R1, static_cast<std::int16_t>(kAddrInputData)));
    REQUIRE(r.rom[1] == encodeRegImm(Opcode::ST, Reg::R1, static_cast<std::int16_t>(kAddrOutputData)));
}

TEST_CASE(".equ constants can be used as LD/ST addresses", "[assembler]") {
    std::string src =
        ".equ COUNTER, 0x200\n"
        "LD R1, [COUNTER]\n"
        "HALT\n";
    AssembleResult r = assemble(src);
    REQUIRE(r.ok);
    REQUIRE(r.rom[0] == encodeRegImm(Opcode::LD, Reg::R1, 0x200));
}

TEST_CASE(".org places code at the requested address", "[assembler]") {
    std::string src =
        ".org 0x010\n"
        "NOP\n";
    AssembleResult r = assemble(src);
    REQUIRE(r.ok);
    REQUIRE(r.rom.size() == 0x011);  // trimmed to the highest used address + 1
    REQUIRE(r.rom[0x010] == encodeNoOperand(Opcode::NOP));
}

TEST_CASE("comments and blank lines are ignored", "[assembler]") {
    AssembleResult r = assemble("; a comment\n\nHALT ; trailing comment\n");
    REQUIRE(r.ok);
    REQUIRE(r.rom.size() == 1);
}

TEST_CASE("unrecognized mnemonic fails closed with a line number", "[assembler]") {
    AssembleResult r = assemble("FROB R1, R2\n");
    REQUIRE_FALSE(r.ok);
    REQUIRE(r.errors[0].line == 1);
}

TEST_CASE("undefined label reference fails closed", "[assembler]") {
    AssembleResult r = assemble("BRA NOWHERE\n");
    REQUIRE_FALSE(r.ok);
}

TEST_CASE("LDI immediate out of range fails closed", "[assembler]") {
    AssembleResult r = assemble("LDI R1, 9999\n");
    REQUIRE_FALSE(r.ok);
}

TEST_CASE("duplicate label fails closed", "[assembler]") {
    AssembleResult r = assemble("A: NOP\nA: NOP\n");
    REQUIRE_FALSE(r.ok);
}

TEST_CASE("empty program fails closed", "[assembler]") {
    AssembleResult r = assemble("; nothing but comments\n");
    REQUIRE_FALSE(r.ok);
}

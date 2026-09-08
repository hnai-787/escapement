#include <catch2/catch_test_macros.hpp>

#include "cpu16/cpu.hpp"

using namespace cpu16;

TEST_CASE("LDI loads an immediate and sets flags", "[cpu]") {
    std::vector<std::uint16_t> rom = {encodeRegImm(Opcode::LDI, Reg::R1, 7), encodeNoOperand(Opcode::HALT)};
    Cpu cpu(rom);
    cpu.step();
    REQUIRE(cpu.registers().r1 == 7);
    REQUIRE_FALSE(cpu.flags().zero);
    REQUIRE_FALSE(cpu.flags().negative);
}

TEST_CASE("ADD computes dst <- dst + src", "[cpu]") {
    std::vector<std::uint16_t> rom = {
        encodeRegImm(Opcode::LDI, Reg::R1, 5),
        encodeRegImm(Opcode::LDI, Reg::R2, 3),
        encodeRegReg(Opcode::ADD, Reg::R1, Reg::R2),
        encodeNoOperand(Opcode::HALT),
    };
    Cpu cpu(rom);
    cpu.step();
    cpu.step();
    cpu.step();
    REQUIRE(cpu.registers().r1 == 8);
}

TEST_CASE("DIV by zero raises a DivideByZero fault and does not write the destination", "[cpu]") {
    std::vector<std::uint16_t> rom = {
        encodeRegImm(Opcode::LDI, Reg::R1, 10),
        encodeRegImm(Opcode::LDI, Reg::R2, 0),
        encodeRegReg(Opcode::DIV, Reg::R1, Reg::R2),
    };
    Cpu cpu(rom);
    cpu.step();
    cpu.step();
    cpu.step();
    REQUIRE(cpu.faulted());
    REQUIRE(cpu.fault() == CpuFault::DivideByZero);
    REQUIRE(cpu.registers().r1 == 10);  // unchanged
}

TEST_CASE("writing to ROM raises a WriteToRom fault", "[cpu]") {
    std::vector<std::uint16_t> rom = {
        encodeRegImm(Opcode::LDI, Reg::R1, 1),
        encodeRegImm(Opcode::ST, Reg::R1, 0x010),  // 0x010 is within ROM
    };
    Cpu cpu(rom);
    cpu.step();
    cpu.step();
    REQUIRE(cpu.faulted());
    REQUIRE(cpu.fault() == CpuFault::WriteToRom);
}

TEST_CASE("RAM load/store round trip", "[cpu]") {
    std::vector<std::uint16_t> rom = {
        encodeRegImm(Opcode::LDI, Reg::R1, 99),
        encodeRegImm(Opcode::ST, Reg::R1, kRamStart + 5),
        encodeRegImm(Opcode::LD, Reg::R2, kRamStart + 5),
        encodeNoOperand(Opcode::HALT),
    };
    Cpu cpu(rom);
    for (int i = 0; i < 3; ++i) cpu.step();
    REQUIRE(cpu.registers().r2 == 99);
}

TEST_CASE("memory-mapped input device supplies queued values in order", "[cpu]") {
    std::vector<std::uint16_t> rom = {
        encodeRegImm(Opcode::LD, Reg::R1, kAddrInputData),
        encodeRegImm(Opcode::LD, Reg::R2, kAddrInputData),
        encodeNoOperand(Opcode::HALT),
    };
    Cpu cpu(rom, {5, 3});
    cpu.step();
    cpu.step();
    REQUIRE(cpu.registers().r1 == 5);
    REQUIRE(cpu.registers().r2 == 3);
}

TEST_CASE("memory-mapped output device appends written values to the output log", "[cpu]") {
    std::vector<std::uint16_t> rom = {
        encodeRegImm(Opcode::LDI, Reg::R1, 8),
        encodeRegImm(Opcode::ST, Reg::R1, kAddrOutputData),
        encodeNoOperand(Opcode::HALT),
    };
    Cpu cpu(rom);
    cpu.step();
    cpu.step();
    REQUIRE(cpu.outputLog() == std::vector<std::int16_t>{8});
}

TEST_CASE("reading the output-only device or writing the input-only device faults", "[cpu]") {
    std::vector<std::uint16_t> rom1 = {encodeRegImm(Opcode::LD, Reg::R1, kAddrOutputData)};
    Cpu cpu1(rom1);
    cpu1.step();
    REQUIRE(cpu1.fault() == CpuFault::InvalidDeviceAccess);

    std::vector<std::uint16_t> rom2 = {encodeRegImm(Opcode::LDI, Reg::R1, 1),
                                        encodeRegImm(Opcode::ST, Reg::R1, kAddrInputData)};
    Cpu cpu2(rom2);
    cpu2.step();
    cpu2.step();
    REQUIRE(cpu2.fault() == CpuFault::InvalidDeviceAccess);
}

TEST_CASE("unmapped MMIO addresses fault", "[cpu]") {
    std::vector<std::uint16_t> rom = {encodeRegImm(Opcode::LD, Reg::R1, 0x3F5)};
    Cpu cpu(rom);
    cpu.step();
    REQUIRE(cpu.fault() == CpuFault::InvalidDeviceAccess);
}

TEST_CASE("HALT stops execution and further step() calls are no-ops", "[cpu]") {
    std::vector<std::uint16_t> rom = {encodeNoOperand(Opcode::HALT), encodeRegImm(Opcode::LDI, Reg::R1, 99)};
    Cpu cpu(rom);
    cpu.step();
    REQUIRE(cpu.halted());
    cpu.step();  // should not execute the LDI that follows
    REQUIRE(cpu.registers().r1 == 0);
}

TEST_CASE("BNZ skips the next instruction when the comparison is non-zero", "[cpu]") {
    // LDI R1,1 ; LDI R2,0 ; CMP R1,R2 (R1-R2=1, not zero) ; BNZ +1 (skip
    // the next instruction) ; LDI R3,99 (skipped) ; LDI R3,1 ; HALT
    std::vector<std::uint16_t> rom2 = {
        encodeRegImm(Opcode::LDI, Reg::R1, 1),
        encodeRegImm(Opcode::LDI, Reg::R2, 0),
        encodeRegReg(Opcode::CMP, Reg::R1, Reg::R2),
        encodeBranch(BranchCond::NotZero, 1),  // skip the next instruction
        encodeRegImm(Opcode::LDI, Reg::R3, 99),
        encodeRegImm(Opcode::LDI, Reg::R3, 1),
        encodeNoOperand(Opcode::HALT),
    };
    Cpu cpu(rom2);
    for (int i = 0; i < 6; ++i) cpu.step();
    REQUIRE(cpu.registers().r3 == 1);  // the LDI R3,99 was skipped
}

TEST_CASE("bus exclusivity: at most one of ROM/RAM/IO select is asserted per bus transaction", "[cpu]") {
    std::vector<std::uint16_t> rom = {
        encodeRegImm(Opcode::LDI, Reg::R1, 1),
        encodeRegImm(Opcode::ST, Reg::R1, kRamStart),
        encodeRegImm(Opcode::LD, Reg::R2, kRamStart),
        encodeRegImm(Opcode::ST, Reg::R1, kAddrOutputData),
        encodeNoOperand(Opcode::HALT),
    };
    Cpu cpu(rom);
    for (int i = 0; i < 5; ++i) {
        InstructionTrace t = cpu.step();
        for (const PhaseRecord& p : t.phases) {
            if (!p.hasBus) continue;
            int selects = (p.bus.control.romSelect ? 1 : 0) + (p.bus.control.ramSelect ? 1 : 0) +
                          (p.bus.control.ioSelect ? 1 : 0);
            REQUIRE(selects <= 1);
        }
    }
}

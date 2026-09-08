#include <catch2/catch_test_macros.hpp>

#include "cpu16/assembler.hpp"
#include "cpu16/cpu.hpp"

using namespace cpu16;

namespace {

std::vector<std::int16_t> runToCompletion(const std::string& source, std::vector<std::int16_t> inputs = {}) {
    AssembleResult r = assemble(source);
    REQUIRE(r.ok);
    Cpu cpu(r.rom, std::move(inputs));
    int guard = 0;
    while (!cpu.halted() && !cpu.faulted() && guard++ < 1000) cpu.step();
    REQUIRE_FALSE(cpu.faulted());
    REQUIRE(cpu.halted());
    return cpu.outputLog();
}

}  // namespace

// These three reproduce, word for word, the documented test cases from
// this project's original x86/Irvine32 implementation (README "Original
// Results"): 5+3=8, 6x7=42, 20/4=5. Running them through the assembler
// and this simulator's real fetch-decode-execute cycle -- rather than a
// direct `add eax, num2` -- is the entire point of this rebuild.

TEST_CASE("regression: 5 + 3 = 8 (matches the original ADD test case)", "[programs]") {
    std::string src =
        "    LDI R1, 5\n"
        "    LDI R2, 3\n"
        "    MOV RES, R1\n"
        "    ADD RES, R2\n"
        "    OUT RES\n"
        "    HALT\n";
    REQUIRE(runToCompletion(src) == std::vector<std::int16_t>{8});
}

TEST_CASE("regression: 6 * 7 = 42 (matches the original MUL test case)", "[programs]") {
    std::string src =
        "    LDI R1, 6\n"
        "    LDI R2, 7\n"
        "    MOV RES, R1\n"
        "    MUL RES, R2\n"
        "    OUT RES\n"
        "    HALT\n";
    REQUIRE(runToCompletion(src) == std::vector<std::int16_t>{42});
}

TEST_CASE("regression: 20 / 4 = 5 (matches the original DIV test case)", "[programs]") {
    std::string src =
        "    LDI R1, 20\n"
        "    LDI R2, 4\n"
        "    MOV RES, R1\n"
        "    DIV RES, R2\n"
        "    OUT RES\n"
        "    HALT\n";
    REQUIRE(runToCompletion(src) == std::vector<std::int16_t>{5});
}

TEST_CASE("calculator reads two operands from the input device, like the original interactive prompts", "[programs]") {
    std::string src =
        "    IN R1\n"
        "    IN R2\n"
        "    MOV RES, R1\n"
        "    ADD RES, R2\n"
        "    OUT RES\n"
        "    HALT\n";
    REQUIRE(runToCompletion(src, {12, 30}) == std::vector<std::int16_t>{42});
}

TEST_CASE("real programmability: a countdown loop using BR, proving this is a general-purpose CPU, not just a calculator command dispatcher",
          "[programs]") {
    // R1 counts down from 5 to 1, OUT each value, then HALT.
    std::string src =
        "    LDI R1, 5\n"
        "LOOP:\n"
        "    OUT R1\n"
        "    LDI R2, 1\n"
        "    SUB R1, R2\n"
        "    BNZ LOOP\n"
        "    HALT\n";
    REQUIRE(runToCompletion(src) == std::vector<std::int16_t>{5, 4, 3, 2, 1});
}

TEST_CASE("real programmability: memory copy via RAM load/store", "[programs]") {
    std::string src =
        "    LDI R1, 77\n"
        "    ST R1, [0x210]\n"
        "    LD R2, [0x210]\n"
        "    OUT R2\n"
        "    HALT\n";
    REQUIRE(runToCompletion(src) == std::vector<std::int16_t>{77});
}

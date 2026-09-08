# cpu16 — Instruction-Set Simulator (Embedded Calculator Processor Design)

## Course Information

| Field | Details |
|---|---|
| Course | Computer Organization and Assembly Language |
| Semester | Semester 3 — Fall 2024 |
| University | Air University, Islamabad |
| Students | Hussain Ali (232095), Sardar Ahmad Ali (232147) |

The original x86/Irvine32 assembly, the architecture diagrams, and the
report/presentation are preserved unmodified under
[`archive/academic-original/`](archive/academic-original/).

## Overview

The original submission diagrammed a real CPU architecture — a Control
Unit, an ALU, a 4-register array (R1, R2, R3, RES), a RAM/ROM memory unit,
and Input/Output blocks, connected by an Address Bus, Data Bus, and
Control Bus (see `diagrams/CPU_BlockDiagram.png` and
`diagrams/BusArchitecture.png`) — and then implemented it as plain 32-bit
x86 assembly that never touches a register file, never fetches an
instruction from memory, and never drives a bus (`add eax, num2` on
Irvine32-provided integers). **cpu16** is a real instruction-set simulator
that actually implements the diagrammed architecture: a genuine
fetch-decode-execute cycle over real 16-bit binary machine words, a
4-register file named exactly as the diagram shows it, a RAM/ROM-split
memory map, and a per-instruction trace exposing real address/data/control
-bus activity.

## Problem Statement

Bridge the gap between processor-architecture diagrams and a working
implementation for real this time: design an instruction set that matches
the documented register/bus model, assemble real programs for it, and
execute them on a simulator whose fetch-decode-execute cycle and bus
signals are directly observable — not implied by a block diagram that the
code never actually follows.

## Design decisions

**Why this is "in the SAP-1/LC-3 tradition," not a copy of either.**
SAP-1 is the closest architectural ancestor (a deliberately tiny,
bus-organized teaching CPU), and LC-3's fixed 16-bit instruction format
with a small opcode field is the closest ISA-design precedent. cpu16
differs from both: 4 directly-addressable registers (not an
accumulator, not 8 registers), a documented RAM/ROM/MMIO memory map, and
a real two-pass assembler. It exists specifically to make *this
project's own diagram* executable, not to reimplement an existing teaching
machine.

**Why a shared bus with address-decoded device selection, not a true
Harvard architecture.** The original diagram draws ROM and RAM as
separate blocks, but both hang off the *same* Address Bus, Data Bus, and
Control Bus — that is not what makes an architecture Harvard (which
requires genuinely separate program/data address spaces or buses). The
correct model for one shared bus feeding multiple devices is address
decoding: the top bits of a 10-bit address select which device
(`ROM_SELECT` / `RAM_SELECT` / `IO_SELECT`) actually drives or captures
the data bus, with `ROM (0x000-0x1FF)`, `RAM (0x200-0x3EF)`, and
memory-mapped `I/O (0x3F0-0x3FF)` as three address ranges of one unified
space. This is documented, tested (`tests/test_cpu.cpp`'s bus-exclusivity
property: at most one device select is ever asserted per transaction),
and matches how real shared-bus systems are built.

**Why memory-mapped I/O instead of dedicated `IN`/`OUT` opcodes.** LC-3
demonstrates the standard idea: device registers occupy ordinary memory
addresses, and ordinary load/store instructions perform I/O. So `IN R1`
and `OUT RES` are assembler **pseudo-instructions** expanding to
`LD R1, [INPUT_DATA]` and `ST RES, [OUTPUT_DATA]` — no separate I/O
opcodes exist in the ISA at all, which is both more minimal and more
architecturally honest.

**Why `MemRead`/`MemWrite`-style signals aren't literally exposed as four
independent physical wires.** With memory-mapped I/O, `IORead`/`IOWrite`
aren't separate physical signals — they're what `READ`/`WRITE` mean
*given* the address decoder selected the I/O range. The trace exposes the
physical-level signals that are actually real in this design
(`READ`, `WRITE`, `ROM_SELECT`, `RAM_SELECT`, `IO_SELECT`); a renderer
can derive the semantic `IORead`/`IOWrite` labels from `IO_SELECT` +
`READ`/`WRITE` when useful, rather than inventing wires that don't exist
in the actual bus architecture.

**Why the full 4-bit opcode space is assigned (no "illegal opcode" is
possible).** The 16 opcodes in `include/cpu16/isa.hpp` fill all 16 values
of the 4-bit opcode field, following LC-3's precedent of a compact,
fully-populated teaching ISA. This was a deliberate choice, not an
oversight — `CpuFault` still has room for an `IllegalOpcode` case in
principle, but it cannot occur given this design, and no test claims it does.

**Why `step()` executes a whole instruction rather than being resumable
mid-phase.** A fully general microcycle simulator would let a caller pause
between FETCH and DECODE across separate calls, which needs the CPU to
persist partial-instruction state between calls. This implementation
instead runs fetch-through-writeback in one `step()` call but *records*
every phase (with its own bus transaction, where relevant) into the
returned trace — so the same bus-level detail the architecture diagram
promises is fully visible in `--trace cycle` output, just computed in one
pass rather than genuinely resumable. Documented as a scope boundary, not
hidden.

## Tools and Technologies

- C++20, CMake, MinGW g++ (same toolchain as the other C++ projects in this workspace)
- [Catch2 v3](https://github.com/catchorg/Catch2) for the test suite (no
  other dependencies — this project has no JSON output, unlike its siblings)

## Instruction Set Summary

16-bit fixed-width instructions, 4-bit opcode, 3 formats:

```text
Register-register  opcode(4) | dst(2)  | src(2)  | reserved(8)
Register-immediate opcode(4) | reg(2)  | imm10/addr10(10, signed for LDI, unsigned address for LD/ST)
Branch              opcode(4) | cond(2) | offset10(10, signed, PC-relative)
```

| Opcode | Mnemonic | Meaning |
|---|---|---|
| 0x0 | NOP | no operation |
| 0x1 | LDI dst, imm | dst <- imm (signed, -512..511) |
| 0x2 | LD reg, [addr] | reg <- Mem[addr] |
| 0x3 | ST reg, [addr] | Mem[addr] <- reg |
| 0x4 | MOV dst, src | dst <- src |
| 0x5 | ADD dst, src | dst <- dst + src |
| 0x6 | SUB dst, src | dst <- dst - src |
| 0x7 | MUL dst, src | dst <- dst * src |
| 0x8 | DIV dst, src | dst <- dst / src (faults on src == 0) |
| 0x9 | AND dst, src | dst <- dst & src |
| 0xA | OR dst, src | dst <- dst \| src |
| 0xB | XOR dst, src | dst <- dst ^ src |
| 0xC | CMP a, b | flags <- a - b (no register write) |
| 0xD | BR cond, offset | conditional PC-relative branch |
| 0xE | NOT dst, src | dst <- ~src |
| 0xF | HALT | stop the CPU |

Pseudo-instructions (assembler sugar, no new opcodes): `IN reg`, `OUT reg`,
`BRA`/`BZ`/`BNZ`/`BN label`.

Memory map (10-bit / 1024-word address space):

```text
0x000-0x1FF  ROM   (program; writes fault WriteToRom)
0x200-0x3EF  RAM   (data)
0x3F0        INPUT_DATA   (memory-mapped input; read-only)
0x3F1        OUTPUT_DATA  (memory-mapped output; write-only)
0x3F2-0x3FF  reserved (any access faults InvalidDeviceAccess)
```

## Features

- Real binary instruction encoding/decoding (`include/cpu16/isa.hpp`) —
  verified against a hand-worked example (`ADD R1, R2` → `0x5100`).
- A genuine fetch-decode-execute `Cpu` (`include/cpu16/cpu.hpp`) with a
  4-register file, Z/N flags, and an architectural fault model
  (`WriteToRom`, `DivideByZero`, `InvalidDeviceAccess`) instead of C++
  exceptions leaking out of the simulated machine.
- A per-instruction trace exposing every phase (FETCH/DECODE/EXECUTE/
  MEMORY/WRITEBACK) with real address-bus, data-bus, and control-signal
  values for phases that touch the bus.
- A two-pass assembler (`include/cpu16/assembler.hpp`) with labels,
  `.org`, `.equ`, comments, and the `IN`/`OUT`/`BRA`/`BZ`/`BNZ`/`BN`
  pseudo-instructions — fails closed on any unrecognized syntax with a
  line number.
- A CLI (`cpu16 assemble` / `cpu16 run`) with `--trace instruction` and
  `--trace cycle` verbosity levels.
- 38 Catch2 test cases / 140 assertions: ISA encode/decode round-trips,
  every opcode's CPU semantics, all three fault types, memory-mapped I/O
  semantics, the bus-exclusivity invariant, the assembler's happy and
  fail-closed paths, and — most importantly — the original project's
  exact documented test cases (5+3=8, 6×7=42, 20÷4=5) reproduced as real
  assembled-and-executed programs.

## Repository Structure

```text
processor-design-embedded-calculator/
  README.md, PROJECT_NOTES.md, CHANGELOG.md
  CMakeLists.txt, build.sh
  include/cpu16/   isa, cpu, assembler, report
  src/             implementations + main.cpp (CLI)
  tests/           38 Catch2 test cases
  examples/        add.asm, multiply.asm, divide.asm, countdown.asm
  archive/academic-original/   original x86 asm, diagrams, report, presentation, untouched
  project.yaml
```

## Building from source

Requires CMake, MinGW g++, and the vcpkg instance already set up for the
sibling C++ projects in this workspace (Catch2 only; no JSON dependency here):

```bash
./build.sh
```

## Usage

```bash
cpu16 assemble program.asm -o program.rom
cpu16 run program.asm [--input N,N,...] [--trace instruction|cycle] [--max-steps N]
```

### Worked example (real, captured output)

`examples/add.asm` reproduces the original's documented `5 + 3 = 8` test
case, with full bus-level tracing:

```text
$ cpu16 run examples/add.asm --trace cycle
PC=0x000  0x1005  LDI R1, 5
  FETCH  AddrBus=0x000 DataBus=0x1005 [READ ROM_SEL ]  IR <- Mem[0x0] = 0x1005
  DECODE  decoded: LDI R1, 5
  EXECUTE  R1 <- 5
  R1=5 R2=0 R3=0 RES=0  Z=0 N=0
...
PC=0x004  0x3ff1  ST RES, [0x3f1]
  FETCH  AddrBus=0x004 DataBus=0x3ff1 [READ ROM_SEL ]  IR <- Mem[0x4] = 0x3ff1
  DECODE  decoded: ST RES, [0x3f1]
  MEMORY  AddrBus=0x3f1 DataBus=0x0008 [WRITE IO_SEL ]  Mem[0x3f1] <- RES
  R1=5 R2=3 R3=0 RES=8  Z=0 N=0
...
--- Final state ---
R1=5 R2=3 R3=0 RES=8  Z=0 N=0
Halted normally.
Output log: 8
```

Note the `OUT RES` pseudo-instruction disassembles back to what it really
is — `ST RES, [0x3f1]` — driving the address bus, data bus, and
`WRITE`/`IO_SEL` control signals exactly as the original bus diagram
claims, which the original x86 implementation never did.

`examples/countdown.asm` proves this is a real, generally-programmable
CPU (branches, loops, flags), not just a calculator command dispatcher —
real, captured output: `Output log: 5 4 3 2 1`.

## How to Review

1. Start with this README, then `diagrams/CPU_BlockDiagram.png` and
   `diagrams/BusArchitecture.png` alongside
   [`include/cpu16/cpu.hpp`](include/cpu16/cpu.hpp) /
   [`src/cpu.cpp`](src/cpu.cpp) to see the diagram-to-code mapping directly.
2. Run `./build.sh` then `./build/cpu16_tests.exe` — 38 test cases, all passing.
3. Run the worked example above with `--trace cycle` and compare against
   the bus diagram.
4. Run `examples/countdown.asm` to see a real loop execute.
5. Compare against `archive/academic-original/code/main.asm` for the
   original x86 implementation.

## Testing

```text
$ ./build/cpu16_tests.exe
All tests passed (140 assertions in 38 test cases)
```

## Original Results (academic artifact)

Reproduced exactly, on real simulated hardware this time — see
`archive/academic-original/` for the original x86 source, diagrams,
report, and presentation:

| NUM1 | NUM2 | Opcode | Result | Reproduced by |
|---|---|---|---|---|
| 5 | 3 | ADD | 8 | `examples/add.asm`, `tests/test_programs.cpp` |
| 6 | 7 | MUL | 42 | `examples/multiply.asm`, `tests/test_programs.cpp` |
| 20 | 4 | DIV | 5 | `examples/divide.asm`, `tests/test_programs.cpp` |

## Limitations

- **16-bit words, 10-bit address space (1024 words)** — a deliberate
  teaching-scale choice, not meant to be a large machine.
- **`step()` is not resumable mid-instruction** — see "Design decisions."
- **No illegal-opcode fault path exists** — the full 4-bit opcode space
  is assigned (deliberate, per LC-3 precedent — see "Design decisions").
- **No stack, no subroutine call/return, no interrupts** — out of scope
  for this teaching CPU (see Future Enhancements).
- **Assembler has no macro system** and reports only the first error per
  assembly run (fails closed immediately rather than accumulating
  multiple errors), matching this workspace's other from-scratch parsers.
- **No CI pipeline has run against this code** — not pushed to GitHub in
  this task.

## Future Enhancements

- A stack pointer and `CALL`/`RET` instructions for subroutines.
- A `--trace json` output format for programmatic inspection/visualization.
- True resumable microcycle stepping (pause between FETCH and DECODE
  across separate API calls), if a future UI wants to single-step at that granularity.
- A disassembler-as-listing mode (`cpu16 disassemble program.rom`).

## Safety and Privacy

No secrets, credentials, or private data are involved.

## Ethical Notice

Academic coursework exercise; the simulator only executes in-memory,
synthetic programs. No ethical concerns apply.

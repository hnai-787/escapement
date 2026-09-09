# Changelog

All notable changes to this project are documented here.
Format loosely follows [Keep a Changelog](https://keepachangelog.com/).

## [Unreleased]

### Added

### Changed

### Fixed

## [1.0.0] - 2026-09-08

### Added

- Rebuilt as **cpu16**, a real instruction-set simulator implementing the
  original diagrammed CPU architecture (control unit, ALU, R1/R2/R3/RES
  register file, RAM/ROM memory unit, address/data/control buses),
  replacing x86/Irvine32 assembly that never touched the design. Original
  source, diagrams, report, and presentation preserved unmodified under
  `archive/original/`.
- A real 16-bit ISA (`include/cpu16/isa.hpp`): 16 opcodes filling the full
  4-bit opcode space, three instruction formats (register-register,
  register-immediate/address, branch), verified against a hand-worked
  encoding example.
- A genuine fetch-decode-execute `Cpu` with a per-instruction trace
  exposing real address-bus, data-bus, and control-signal (READ/WRITE/
  ROM_SELECT/RAM_SELECT/IO_SELECT) activity for every phase.
- A memory map with ROM (0x000-0x1FF), RAM (0x200-0x3EF), and
  memory-mapped I/O (0x3F0-0x3FF), modeled as one shared bus with
  address-decoded device selection -- not a true Harvard architecture,
  since the original diagram shows ROM and RAM on the same buses.
- An architectural fault model (`WriteToRom`, `DivideByZero`,
  `InvalidDeviceAccess`) instead of C++ exceptions leaking out of the
  simulated machine.
- A two-pass assembler with labels, `.org`, `.equ`, comments, and
  `IN`/`OUT`/`BRA`/`BZ`/`BNZ`/`BN` pseudo-instructions, failing closed on
  any unrecognized syntax with a line number.
- A CLI (`cpu16 assemble` / `cpu16 run`) with `--trace instruction` and
  `--trace cycle` verbosity levels.
- 38 Catch2 test cases / 140 assertions, including the original project's
  exact documented test cases (5+3=8, 6x7=42, 20/4=5) reproduced as real
  assembled-and-executed programs, plus a countdown-loop example proving
  general programmability beyond calculator dispatch.

# Project Notes

## Source

Migrated from `air-university-cybersecurity-projects/projects/processor-design-embedded-calculator`
into this workspace as an independent project on 2026-09-07.

## Cleanup decisions

- `exports/` and `screenshots/` were empty placeholder folders (only
  `.gitkeep`) in the source repo and were copied as-is.

## Assumptions

- Co-author "Sardar Ahmad Ali (232147)" is credited in
  `docs/COAL_Final_Report.docx`. The course-info table that used to
  surface this in the README was later removed along with other academic
  framing.

## Remaining work

- Consider adding real screenshots of the assembled program running, since
  the `screenshots/` folder is currently empty.

## 2026-09-08: Rebuilt as cpu16 (instruction-set simulator)

### What changed and why

The core problem with the original submission: the architecture diagrams
(`diagrams/CPU_BlockDiagram.png`, `diagrams/BusArchitecture.png`) describe
a real CPU -- control unit, ALU, a 4-register array named R1/R2/R3/RES, a
RAM/ROM memory unit, address/data/control buses -- but the actual x86
assembly implementation never touches any of it; it just calls
`ReadInt`/`add eax, num2`/`WriteInt` from the Irvine32 library. The
diagram and the code had no relationship. This rebuild makes the diagram
executable: a real 16-bit ISA matching the register file exactly, a
genuine fetch-decode-execute cycle, and a bus trace showing real
address/data/control signal activity per instruction phase.

Note: `diagrams/DataPathDiagram.png` (the BIU/EU, AH/AL, instruction-queue
diagram) is a generic Intel-8086-style reference diagram included in the
original report for context/comparison -- it is not this project's own
custom design. The actual designed architecture this rebuild implements
is the CPU block diagram and bus architecture diagram only.

### Key engineering decisions and why

- **Shared bus with address decoding, not Harvard architecture.** The
  diagram draws ROM and RAM as separate blocks but connects both to the
  *same* address/data/control buses. A true Harvard architecture needs
  genuinely separate program/data buses or address spaces; this design
  instead uses one 10-bit address space with `ROM_SELECT`/`RAM_SELECT`/
  `IO_SELECT` derived from the address range, which is the standard,
  correct way to model multiple devices sharing one bus.
- **Memory-mapped I/O, no dedicated IN/OUT opcodes.** Following LC-3's
  precedent: device registers occupy ordinary memory addresses, and
  ordinary LD/ST instructions perform I/O. `IN`/`OUT` exist only as
  assembler pseudo-instructions expanding to `LD`/`ST` against
  `INPUT_DATA`/`OUTPUT_DATA` -- no extra ISA complexity needed.
- **`step()` executes one whole instruction, not a resumable
  microcycle.** A genuinely resumable pause-between-phases API would
  need the CPU to persist partial-instruction state across calls, for
  modest additional teaching value over what a rich per-instruction
  phase trace already provides. Documented as a deliberate scope
  boundary, not a hidden limitation -- the same bus-level detail is
  fully visible in the trace either way.
- **All 16 opcode values are assigned** (following LC-3's fully-populated
  compact opcode space), meaning there is no reachable "illegal opcode"
  fault in this design -- a deliberate choice, not an oversight.
- **Register-register ops use 2-operand (dst <- dst OP src) semantics**,
  matching the exact worked encoding example from this project's own
  research pass (`ADD R1, R2` -> `0x5100`), verified as a unit test.

### Verification performed

`cmake --build` and the full test suite (38 test cases / 140 assertions,
all passing) were actually run. The CLI was run with `--trace cycle`
against the original's exact documented ADD test case and the output was
hand-checked against both the bus architecture diagram (real
`ROM_SELECT`/`IO_SELECT`/`READ`/`WRITE` signals appear exactly where
expected) and the original's claimed result (5+3=8). The MUL (6x7=42) and
DIV (20/4=5) cases were reproduced identically. A countdown-loop example
program was run to confirm this is a genuinely programmable CPU (branches,
loops, flags), not just a calculator command dispatcher in disguise --
real captured output: `5 4 3 2 1`.

### Remaining work / honest limitations

See README "Limitations" and "Future Enhancements" -- notably: no stack
or subroutine calls, no true resumable microcycle stepping, assembler
reports only the first error per run, and no CI pipeline has run against
this code (not pushed to GitHub in this task).

# Processor Design — Embedded Calculator

## Course Information

| Field | Details |
|---|---|
| Course | Computer Organization and Assembly Language |
| Semester | Semester 3 — Fall 2024 |
| University | Air University, Islamabad |
| Students | Hussain Ali (232095), Sardar Ahmad Ali (232147) |

## Overview

Designs a simple processor architecture — control unit, ALU, registers, and
memory interface — for a basic calculator, then implements it as real
32-bit x86 assembly using the Irvine32 library.

## Problem Statement

Bridge the gap between processor-architecture theory (control unit, ALU,
bus design) and a working implementation, by both diagramming the
architecture and implementing it in assembly.

## Objectives

- Design a bus architecture (bidirectional data bus, unidirectional address/control buses).
- Implement ADD/SUB/MUL/DIV/AND/OR opcodes in x86 assembly.
- Handle divide-by-zero and invalid-opcode cases safely.

## Tools and Technologies

- x86 assembly (MASM, `.386`, flat model, Irvine32 library)
- Visual Studio (MASM toolchain)
- EdrawMax (architecture diagrams)

## Features

- ADD, SUB, MUL, DIV, AND, OR opcodes.
- Divide-by-zero and invalid-opcode handling.
- Documented bus/data-path architecture matching the implementation.

## Methodology

1. Design the control unit, ALU, register set, and bus architecture.
2. Diagram the block, bus, and data-path architecture (`diagrams/`).
3. Implement the calculator loop in x86 assembly (`code/main.asm`).
4. Test each opcode plus divide-by-zero and invalid-opcode cases.

## Repository Structure

```text
processor-design-embedded-calculator/
  README.md
  PROJECT_NOTES.md
  code/
    main.asm
  diagrams/
    BusArchitecture.png
    CPU_BlockDiagram.png
    DataPathDiagram.png
  docs/COAL_Final_Report.docx
  presentation/COAL_Presentation.pptx
  project.yaml
```

## Setup Instructions

Requires MASM32 / Visual Studio with the Irvine32 library linked.

```bash
ml /c /coff code/main.asm
link /subsystem:console main.obj
```

## Usage

```bash
main.exe
# prompts for NUM1, NUM2, and an opcode (ADD/SUB/MUL/DIV/AND/OR)
```

## How to Review

1. Start with this README, then `docs/COAL_Final_Report.docx`.
2. Review `diagrams/` for the architecture.
3. Read `code/main.asm` — the `processorLoop` implements the documented architecture.

## Screenshots

Architecture diagrams are in `diagrams/`; the project's `screenshots/`
folder was a placeholder in the source material (no screenshots were
captured for this project — see `docs/COAL_Final_Report.docx` for the
embedded test-run output instead).

## Results

Documented and verified test cases, matched between `code/main.asm` and
the report:

| NUM1 | NUM2 | Opcode | Result | Outcome |
|---|---|---|---|---|
| 5 | 3 | ADD | 8 | Pass |
| 6 | 7 | MUL | 42 | Pass |
| 20 | 4 | DIV | 5 | Pass |
| — | — | invalid opcode | "Invalid Operation" | Pass |

## Limitations

- Single calculator loop, no persistent state or memory-mapped I/O beyond the documented architecture.
- Educational scope — not a synthesizable hardware design.

## Future Enhancements

- Extend the opcode set (e.g. modulo, bitwise shifts).
- Simulate the designed bus architecture in a hardware description language for comparison.

## Safety and Privacy

No secrets, credentials, or private data are involved.

## Ethical Notice

Academic coursework exercise; no ethical concerns apply.

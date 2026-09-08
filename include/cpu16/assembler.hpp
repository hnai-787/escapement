#ifndef CPU16_ASSEMBLER_HPP
#define CPU16_ASSEMBLER_HPP

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace cpu16 {

struct AssembleError {
    int line;
    std::string message;
};

struct AssembleResult {
    bool ok = false;
    std::vector<std::uint16_t> rom;
    std::vector<AssembleError> errors;
    std::map<std::string, int> labels;  // for listings/diagnostics
};

// A small two-pass assembler for the ISA in isa.hpp. Supports labels,
// `.org <addr>`, `.equ NAME value`, `.word <value>`, comments (`;`), and
// the pseudo-instructions `IN reg` / `OUT reg` (memory-mapped I/O sugar
// for `LD reg, [INPUT_DATA]` / `ST reg, [OUTPUT_DATA]`) and `BRA`/`BZ`/
// `BNZ`/`BN label` (sugar for `BR` with the matching condition and a
// computed PC-relative offset). `INPUT_DATA` and `OUTPUT_DATA` are
// predefined constants. Fails closed: any unrecognized syntax is a hard
// error with a line number, never silently skipped (see the sibling
// fwlint/numsolve/bracketsched projects for the same design principle
// applied to their own parsers).
AssembleResult assemble(const std::string& source);

}  // namespace cpu16

#endif

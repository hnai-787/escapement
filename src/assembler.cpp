#include "cpu16/assembler.hpp"

#include <algorithm>
#include <cctype>
#include <optional>
#include <sstream>

#include "cpu16/cpu.hpp"
#include "cpu16/isa.hpp"

namespace cpu16 {

namespace {

std::string trim(const std::string& s) {
    std::size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    std::size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

std::string stripComment(const std::string& line) {
    std::size_t pos = line.find(';');
    return pos == std::string::npos ? line : line.substr(0, pos);
}

std::string toUpper(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::toupper(c); });
    return s;
}

std::vector<std::string> splitOperands(const std::string& raw) {
    std::vector<std::string> result;
    std::string current;
    for (char c : raw) {
        if (c == ',') {
            result.push_back(trim(current));
            current.clear();
        } else {
            current += c;
        }
    }
    if (!trim(current).empty() || !result.empty()) result.push_back(trim(current));
    return result;
}

std::string stripBrackets(const std::string& s) {
    std::string t = trim(s);
    if (!t.empty() && t.front() == '[' && t.back() == ']') return trim(t.substr(1, t.size() - 2));
    return t;
}

std::optional<int> parseIntLiteral(const std::string& tok) {
    if (tok.empty()) return std::nullopt;
    try {
        std::size_t idx = 0;
        if (tok.size() > 2 && tok[0] == '0' && (tok[1] == 'x' || tok[1] == 'X')) {
            long v = std::stol(tok.substr(2), &idx, 16);
            if (idx != tok.size() - 2) return std::nullopt;
            return static_cast<int>(v);
        }
        long v = std::stol(tok, &idx, 10);
        if (idx != tok.size()) return std::nullopt;
        return static_cast<int>(v);
    } catch (...) {
        return std::nullopt;
    }
}

struct ParsedLine {
    std::string label;  // empty if none
    std::string mnemonic;  // empty if this line is only a label or blank
    std::string operandsRaw;
    bool isBlank = false;
};

ParsedLine parseLine(const std::string& rawLine) {
    ParsedLine result;
    std::string line = trim(stripComment(rawLine));
    if (line.empty()) {
        result.isBlank = true;
        return result;
    }

    std::size_t colon = line.find(':');
    if (colon != std::string::npos) {
        result.label = trim(line.substr(0, colon));
        line = trim(line.substr(colon + 1));
        if (line.empty()) return result;
    }

    std::size_t space = line.find_first_of(" \t");
    if (space == std::string::npos) {
        result.mnemonic = toUpper(line);
    } else {
        result.mnemonic = toUpper(line.substr(0, space));
        result.operandsRaw = trim(line.substr(space + 1));
    }
    return result;
}

bool isKnownMnemonic(const std::string& m) {
    static const std::vector<std::string> known = {
        "NOP", "HALT", "LDI", "LD", "ST", "MOV", "ADD", "SUB", "MUL", "DIV",
        "AND", "OR",   "XOR", "CMP", "NOT", "BRA", "BZ", "BNZ", "BN", "IN", "OUT",
    };
    return std::find(known.begin(), known.end(), m) != known.end();
}

}  // namespace

AssembleResult assemble(const std::string& source) {
    AssembleResult result;
    std::vector<std::string> rawLines;
    {
        std::istringstream stream(source);
        std::string line;
        while (std::getline(stream, line)) rawLines.push_back(line);
    }

    std::vector<ParsedLine> lines;
    lines.reserve(rawLines.size());
    for (const std::string& raw : rawLines) lines.push_back(parseLine(raw));

    std::map<std::string, int> constants = {
        {"INPUT_DATA", static_cast<int>(kAddrInputData)},
        {"OUTPUT_DATA", static_cast<int>(kAddrOutputData)},
    };
    std::map<std::string, int>& labels = result.labels;

    constexpr int kRomCapacity = kRomEnd - kRomStart + 1;

    // ---- Pass 1: addresses, labels, constants ----
    int address = 0;
    for (std::size_t i = 0; i < lines.size(); ++i) {
        const ParsedLine& pl = lines[i];
        int lineNumber = static_cast<int>(i) + 1;
        if (pl.isBlank) continue;

        if (!pl.label.empty()) {
            if (labels.count(pl.label) || constants.count(pl.label)) {
                result.errors.push_back({lineNumber, "duplicate label \"" + pl.label + "\""});
                return result;
            }
            labels[pl.label] = address;
        }
        if (pl.mnemonic.empty()) continue;

        if (pl.mnemonic == ".ORG") {
            auto value = parseIntLiteral(trim(pl.operandsRaw));
            if (!value || *value < 0 || *value >= kRomCapacity) {
                result.errors.push_back({lineNumber, ".org requires a valid address within 0x000-0x1FF"});
                return result;
            }
            address = *value;
        } else if (pl.mnemonic == ".EQU") {
            std::vector<std::string> ops = splitOperands(pl.operandsRaw);
            if (ops.size() != 2) {
                result.errors.push_back({lineNumber, ".equ requires NAME, value"});
                return result;
            }
            auto value = parseIntLiteral(ops[1]);
            if (!value) {
                result.errors.push_back({lineNumber, ".equ value must be a numeric literal"});
                return result;
            }
            constants[ops[0]] = *value;
        } else if (pl.mnemonic == ".WORD") {
            if (address >= kRomCapacity) {
                result.errors.push_back({lineNumber, "program exceeds ROM capacity (512 words)"});
                return result;
            }
            ++address;
        } else if (isKnownMnemonic(pl.mnemonic)) {
            if (address >= kRomCapacity) {
                result.errors.push_back({lineNumber, "program exceeds ROM capacity (512 words)"});
                return result;
            }
            ++address;
        } else {
            result.errors.push_back({lineNumber, "unrecognized mnemonic or directive \"" + pl.mnemonic + "\""});
            return result;
        }
    }

    // ---- Pass 2: encode ----
    std::vector<std::uint16_t> image(kRomCapacity, 0);
    bool anyEmitted = false;
    int maxUsed = -1;
    address = 0;

    auto resolve = [&](const std::string& tok, int lineNumber, int& out) -> bool {
        if (auto lit = parseIntLiteral(tok)) {
            out = *lit;
            return true;
        }
        if (auto it = constants.find(tok); it != constants.end()) {
            out = it->second;
            return true;
        }
        if (auto it = labels.find(tok); it != labels.end()) {
            out = it->second;
            return true;
        }
        result.errors.push_back({lineNumber, "undefined symbol \"" + tok + "\""});
        return false;
    };
    auto resolveReg = [&](const std::string& tok, int lineNumber, Reg& out) -> bool {
        if (auto r = regFromName(toUpper(trim(tok)))) {
            out = *r;
            return true;
        }
        result.errors.push_back({lineNumber, "expected a register (R1/R2/R3/RES), got \"" + tok + "\""});
        return false;
    };

    for (std::size_t i = 0; i < lines.size(); ++i) {
        const ParsedLine& pl = lines[i];
        int lineNumber = static_cast<int>(i) + 1;
        if (pl.isBlank) continue;
        if (pl.mnemonic.empty()) continue;

        if (pl.mnemonic == ".ORG") {
            int value = 0;
            resolve(trim(pl.operandsRaw), lineNumber, value);  // already validated in pass 1
            address = value;
            continue;
        }
        if (pl.mnemonic == ".EQU") continue;  // handled in pass 1

        std::uint16_t word = 0;
        std::vector<std::string> ops = splitOperands(pl.operandsRaw);

        if (pl.mnemonic == ".WORD") {
            int value = 0;
            if (ops.size() != 1 || !resolve(ops[0], lineNumber, value)) return result;
            word = static_cast<std::uint16_t>(value);
        } else if (pl.mnemonic == "NOP") {
            word = encodeNoOperand(Opcode::NOP);
        } else if (pl.mnemonic == "HALT") {
            word = encodeNoOperand(Opcode::HALT);
        } else if (pl.mnemonic == "LDI") {
            Reg dst;
            int imm = 0;
            if (ops.size() != 2 || !resolveReg(ops[0], lineNumber, dst) || !resolve(ops[1], lineNumber, imm)) {
                return result;
            }
            if (imm < -512 || imm > 511) {
                result.errors.push_back({lineNumber, "LDI immediate must fit in a signed 10-bit field (-512..511)"});
                return result;
            }
            word = encodeRegImm(Opcode::LDI, dst, static_cast<std::int16_t>(imm));
        } else if (pl.mnemonic == "LD" || pl.mnemonic == "ST") {
            Reg reg;
            int addr = 0;
            if (ops.size() != 2 || !resolveReg(ops[0], lineNumber, reg) ||
                !resolve(stripBrackets(ops[1]), lineNumber, addr)) {
                return result;
            }
            if (addr < 0 || addr > 1023) {
                result.errors.push_back({lineNumber, "address must be within 0x000-0x3FF"});
                return result;
            }
            word = encodeRegImm(pl.mnemonic == "LD" ? Opcode::LD : Opcode::ST, reg,
                                 static_cast<std::int16_t>(addr));
        } else if (pl.mnemonic == "IN" || pl.mnemonic == "OUT") {
            Reg reg;
            if (ops.size() != 1 || !resolveReg(ops[0], lineNumber, reg)) return result;
            word = pl.mnemonic == "IN" ? encodeRegImm(Opcode::LD, reg, static_cast<std::int16_t>(kAddrInputData))
                                       : encodeRegImm(Opcode::ST, reg, static_cast<std::int16_t>(kAddrOutputData));
        } else if (pl.mnemonic == "BRA" || pl.mnemonic == "BZ" || pl.mnemonic == "BNZ" || pl.mnemonic == "BN") {
            int target = 0;
            if (ops.size() != 1 || !resolve(ops[0], lineNumber, target)) return result;
            int offset = target - (address + 1);
            if (offset < -512 || offset > 511) {
                result.errors.push_back({lineNumber, "branch target is out of PC-relative range (-512..511)"});
                return result;
            }
            BranchCond cond = pl.mnemonic == "BRA"  ? BranchCond::Always
                               : pl.mnemonic == "BZ" ? BranchCond::Zero
                               : pl.mnemonic == "BNZ" ? BranchCond::NotZero
                                                       : BranchCond::Negative;
            word = encodeBranch(cond, static_cast<std::int16_t>(offset));
        } else if (isKnownMnemonic(pl.mnemonic)) {
            // Register-register format: MOV/ADD/SUB/MUL/DIV/AND/OR/XOR/CMP/NOT
            auto op = opcodeFromName(pl.mnemonic);
            Reg dst, src;
            if (!op || ops.size() != 2 || !resolveReg(ops[0], lineNumber, dst) ||
                !resolveReg(ops[1], lineNumber, src)) {
                result.errors.push_back({lineNumber, "malformed instruction \"" + pl.mnemonic + "\""});
                return result;
            }
            word = encodeRegReg(*op, dst, src);
        } else {
            result.errors.push_back({lineNumber, "unrecognized mnemonic \"" + pl.mnemonic + "\""});
            return result;
        }

        if (!result.errors.empty()) return result;
        image[address] = word;
        maxUsed = address;
        anyEmitted = true;
        ++address;
    }

    if (!anyEmitted) {
        result.errors.push_back({0, "program is empty (no instructions or .word directives)"});
        return result;
    }

    result.rom.assign(image.begin(), image.begin() + maxUsed + 1);
    result.ok = true;
    return result;
}

}  // namespace cpu16

#include <fstream>
#include <iostream>
#include <sstream>

#include "cpu16/assembler.hpp"
#include "cpu16/cpu.hpp"
#include "cpu16/report.hpp"

using namespace cpu16;

namespace {

void printHelp() {
    std::cout <<
        "cpu16 -- assembler and simulator for this project's custom CPU\n\n"
        "USAGE:\n"
        "  cpu16 assemble <in.asm> -o <out.rom>\n"
        "  cpu16 run <in.asm|in.rom> [--input N,N,...] [--trace instruction|cycle] [--max-steps N]\n"
        "  cpu16 --help\n";
}

std::string readFile(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

bool writeRomFile(const std::string& path, const std::vector<std::uint16_t>& rom) {
    std::ofstream f(path);
    if (!f.is_open()) return false;
    for (std::uint16_t word : rom) f << std::hex << word << "\n";
    return true;
}

std::vector<std::uint16_t> readRomFile(const std::string& path) {
    std::vector<std::uint16_t> rom;
    std::ifstream f(path);
    std::string line;
    while (std::getline(f, line)) {
        if (line.empty()) continue;
        rom.push_back(static_cast<std::uint16_t>(std::stoul(line, nullptr, 16)));
    }
    return rom;
}

bool endsWith(const std::string& s, const std::string& suffix) {
    return s.size() >= suffix.size() && s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
}

int cmdAssemble(int argc, char* argv[]) {
    if (argc < 1) {
        std::cerr << "Error: assemble requires <in.asm>\n";
        return 2;
    }
    std::string outPath;
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "-o" && i + 1 < argc) outPath = argv[++i];
    }
    AssembleResult result = assemble(readFile(argv[0]));
    if (!result.ok) {
        std::cerr << "Assembly failed:\n";
        for (const AssembleError& e : result.errors) std::cerr << "  line " << e.line << ": " << e.message << "\n";
        return 2;
    }
    std::cout << "Assembled " << result.rom.size() << " word(s).\n";
    if (!outPath.empty()) {
        if (!writeRomFile(outPath, result.rom)) {
            std::cerr << "Error: could not write " << outPath << "\n";
            return 2;
        }
        std::cout << "Wrote " << outPath << "\n";
    }
    return 0;
}

int cmdRun(int argc, char* argv[]) {
    if (argc < 1) {
        std::cerr << "Error: run requires <in.asm|in.rom>\n";
        return 2;
    }
    std::string path = argv[0];
    std::string traceMode;
    std::vector<std::int16_t> inputs;
    int maxSteps = 10000;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--trace" && i + 1 < argc) {
            traceMode = argv[++i];
        } else if (arg == "--input" && i + 1 < argc) {
            std::istringstream ss(argv[++i]);
            std::string tok;
            while (std::getline(ss, tok, ',')) inputs.push_back(static_cast<std::int16_t>(std::stoi(tok)));
        } else if (arg == "--max-steps" && i + 1 < argc) {
            maxSteps = std::stoi(argv[++i]);
        } else {
            std::cerr << "Error: unrecognized option \"" << arg << "\"\n";
            return 2;
        }
    }

    std::vector<std::uint16_t> rom;
    if (endsWith(path, ".asm")) {
        AssembleResult result = assemble(readFile(path));
        if (!result.ok) {
            std::cerr << "Assembly failed:\n";
            for (const AssembleError& e : result.errors) std::cerr << "  line " << e.line << ": " << e.message << "\n";
            return 2;
        }
        rom = result.rom;
    } else {
        rom = readRomFile(path);
    }

    Cpu cpu(rom, inputs);
    int steps = 0;
    while (!cpu.halted() && !cpu.faulted() && steps < maxSteps) {
        InstructionTrace trace = cpu.step();
        if (traceMode == "cycle") {
            std::cout << renderCycleDetail(trace);
        } else if (traceMode == "instruction") {
            std::cout << renderInstructionSummary(trace) << "\n";
        }
        ++steps;
    }

    std::cout << "\n--- Final state ---\n";
    std::cout << renderRegisters(cpu.registers()) << "  " << renderFlags(cpu.flags()) << "\n";
    std::cout << "PC=0x" << std::hex << cpu.pc() << std::dec << "\n";
    if (cpu.faulted()) {
        std::cout << "FAULT: " << faultName(cpu.fault()) << "\n";
    } else if (cpu.halted()) {
        std::cout << "Halted normally.\n";
    } else {
        std::cout << "Stopped: max step count (" << maxSteps << ") reached without HALT.\n";
    }
    std::cout << "Output log:";
    for (std::int16_t v : cpu.outputLog()) std::cout << " " << v;
    std::cout << "\n";

    return cpu.faulted() ? 1 : 0;
}

}  // namespace

int main(int argc, char* argv[]) {
    if (argc < 2 || std::string(argv[1]) == "--help" || std::string(argv[1]) == "-h") {
        printHelp();
        return argc < 2 ? 2 : 0;
    }
    std::string command = argv[1];
    if (command == "assemble") return cmdAssemble(argc - 2, argv + 2);
    if (command == "run") return cmdRun(argc - 2, argv + 2);

    std::cerr << "Error: unrecognized command \"" << command << "\"\n\n";
    printHelp();
    return 2;
}

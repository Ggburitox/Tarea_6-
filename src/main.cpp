#include <iostream>
#include <sstream>
#include <string>
#include <iomanip>
#include "memory.h"
#include "cpu.h"

static void printHelp() {
    std::cout <<
        "\n--- Comandos disponibles ---\n"
        "  step [N]           Ejecuta N instrucciones (default 1)\n"
        "  run                Ejecuta hasta el final\n"
        "  pc                 Muestra el Program Counter\n"
        "  regs               Muestra todos los registros\n"
        "  regs x5 a0 sp ...  Muestra registros especificos\n"
        "  mem ADDR [END]     Muestra memoria en hex (ej: mem 0x1000 0x101c)\n"
        "  mem ADDR           Muestra 16 bytes desde ADDR\n"
        "  memw ADDR [N]      Muestra N palabras (32-bit) desde ADDR\n"
        "  load ARCHIVO.bin   Carga un programa binario\n"
        "  reset              Reinicia PC y registros\n"
        "  help               Esta ayuda\n"
        "  exit / quit        Salir\n"
        "----------------------------\n\n";
}

static uint32_t parseHex(const std::string& s) {
    // Accept "0x1000" or "1000"
    return (uint32_t)std::stoul(s, nullptr, 0);
}

int main(int argc, char* argv[]) {
    Memory mem;
    CPU    cpu(mem);

    std::cout << "==============================================\n";
    std::cout << "     Simulador RISC-V RV32I\n";
    std::cout << "==============================================\n";
    std::cout << "Escribe 'help' para ver los comandos.\n\n";

    // Load binary from command-line argument if provided
    if (argc >= 2) {
        std::string path = argv[1];
        if (mem.loadFromFile(path)) {
            cpu.reset();
            std::cout << "Listo. PC = 0x00000000\n\n";
        } else {
            return 1;
        }
    }

    std::string line;
    while (true) {
        std::cout << "> ";
        std::cout.flush();
        if (!std::getline(std::cin, line)) {
            std::cout << "\nSee you next time...\n";
            break;
        }

        // Trim leading spaces
        size_t start = line.find_first_not_of(" \t");
        if (start == std::string::npos) continue;
        line = line.substr(start);
        if (line.empty()) continue;

        std::istringstream ss(line);
        std::string cmd;
        ss >> cmd;

        // ── exit / quit ────────────────────────────────────────────────────
        if (cmd == "exit" || cmd == "quit") {
            std::cout << "See you next time...\n";
            break;
        }

        // ── help ──────────────────────────────────────────────────────────
        else if (cmd == "help") {
            printHelp();
        }

        // ── step [N] ──────────────────────────────────────────────────────
        else if (cmd == "step" || cmd == "s") {
            int n = 1;
            std::string arg;
            if (ss >> arg) {
                try { n = std::stoi(arg); } catch (...) { n = 1; }
            }
            int executed = 0;
            for (int k = 0; k < n; k++) {
                if (!cpu.step()) break;
                executed++;
            }
            std::cout << "Ejecutadas " << executed << " instruccion(es). "
                      << "PC = 0x" << std::hex << std::setfill('0')
                      << std::setw(8) << cpu.getPC() << "\n" << std::dec;
        }

        // ── run ───────────────────────────────────────────────────────────
        else if (cmd == "run" || cmd == "r") {
            std::cout << "Ejecutando programa...\n";
            cpu.run();
            std::cout << "Programa terminado. PC = 0x"
                      << std::hex << std::setfill('0') << std::setw(8)
                      << cpu.getPC() << "\n" << std::dec;
        }

        // ── pc ────────────────────────────────────────────────────────────
        else if (cmd == "pc") {
            std::cout << "pc = 0x" << std::hex << std::setfill('0')
                      << std::setw(8) << cpu.getPC() << "\n" << std::dec;
        }

        // ── regs [list] ───────────────────────────────────────────────────
        else if (cmd == "regs" || cmd == "reg") {
            std::string rest;
            std::getline(ss, rest);
            // Strip leading space
            if (!rest.empty() && rest[0] == ' ') rest = rest.substr(1);

            if (rest.empty()) {
                cpu.printAllRegs();
            } else {
                cpu.printRegs(rest);
            }
        }

        // ── mem ADDR [END] ────────────────────────────────────────────────
        else if (cmd == "mem") {
            std::string s1, s2;
            if (!(ss >> s1)) {
                std::cerr << "Uso: mem ADDR [END]\n";
                continue;
            }
            try {
                uint32_t start = parseHex(s1);
                uint32_t end;
                if (ss >> s2) end = parseHex(s2);
                else          end = start + 15;
                mem.dump(start, end);
            } catch (...) {
                std::cerr << "Direccion invalida.\n";
            }
        }

        // ── memw ADDR [N] - show N words ──────────────────────────────────
        else if (cmd == "memw") {
            std::string s1, s2;
            if (!(ss >> s1)) {
                std::cerr << "Uso: memw ADDR [N]\n";
                continue;
            }
            try {
                uint32_t addr = parseHex(s1);
                int n = 4;
                if (ss >> s2) n = std::stoi(s2);
                std::cout << std::hex << std::setfill('0');
                std::cout << "Memoria (words desde 0x" << std::setw(8) << addr << "):\n";
                for (int k = 0; k < n; k++) {
                    uint32_t a = addr + k * 4;
                    std::cout << "  [0x" << std::setw(8) << a << "] = 0x"
                              << std::setw(8) << mem.readWord(a)
                              << "  (" << std::dec << (int32_t)mem.readWord(a)
                              << ")\n" << std::hex;
                }
                std::cout << std::dec;
            } catch (...) {
                std::cerr << "Error al leer memoria.\n";
            }
        }

        // ── load FILE ─────────────────────────────────────────────────────
        else if (cmd == "load") {
            std::string filename;
            if (!(ss >> filename)) {
                std::cerr << "Uso: load ARCHIVO.bin\n";
            } else {
                if (mem.loadFromFile(filename)) {
                    cpu.reset();
                    std::cout << "Listo. PC = 0x00000000\n";
                }
            }
        }

        // ── reset ─────────────────────────────────────────────────────────
        else if (cmd == "reset") {
            cpu.reset();
            std::cout << "CPU reiniciada. PC = 0x00000000\n";
        }

        // ── unknown ───────────────────────────────────────────────────────
        else {
            std::cerr << "Comando desconocido: '" << cmd
                      << "'. Escribe 'help'.\n";
        }
    }

    return 0;
}

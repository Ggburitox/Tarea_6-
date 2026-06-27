#include "cpu.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <stdexcept>

CPU::CPU(Memory& mem) : mem(mem) {
    reset();
}

void CPU::reset() {
    pc = 0x00000000;
    for (int i = 0; i < 32; i++) regs[i] = 0;
    regs[2] = Memory::MEM_SIZE - 4;
    state = CPUState::RUNNING;
}

void CPU::writeReg(int rd, uint32_t val) {
    if (rd != 0) regs[rd] = val;
}

uint32_t CPU::getReg(int r) const {
    return (r >= 0 && r < 32) ? regs[r] : 0;
}

// ── Main execution loop ──

bool CPU::step() {
    if (state != CPUState::RUNNING) {
        std::cout << "CPU detenida. Usa 'reset' para reiniciar.\n";
        return false;
    }

    // FETCH
    uint32_t raw;
    try {
        raw = mem.readWord(pc);
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] No se pudo leer instruccion en PC=0x"
                  << std::hex << pc << ": " << e.what() << "\n";
        state = CPUState::ERROR;
        return false;
    }

    // All-zero word = end sentinel (common in these binaries)
    if (raw == 0x00000000) {
        std::cout << "[FIN] Instruccion nula en PC=0x"
                  << std::hex << pc << " - programa terminado.\n";
        state = CPUState::HALTED;
        return false;
    }

    // DECODE
    Instruction instr = decode(raw);

    if (instr.type == InstrType::UNKNOWN) {
        // 0xAAAAAAAA is filler in these test binaries, also means end
        if (raw == 0xAAAAAAAA) {
            std::cout << "[FIN] Fin del programa en PC=0x"
                      << std::hex << pc << "\n";
            state = CPUState::HALTED;
            return false;
        }
        std::cerr << "[ERROR] Instruccion invalida 0x" << std::hex << raw
                  << " en PC=0x" << pc << "\n";
        state = CPUState::ERROR;
        return false;
    }

    uint32_t cur_pc = pc;
    pc += 4;

    // EXECUTE
    try {
        switch (instr.type) {
            case InstrType::R: execR(instr);              break;
            case InstrType::I: execI(instr, cur_pc);      break;
            case InstrType::S: execS(instr);              break;
            case InstrType::B: execB(instr, cur_pc);      break;
            case InstrType::U: execU(instr, cur_pc);      break;
            case InstrType::J: execJ(instr, cur_pc);      break;
            default: break;
        }
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] " << e.what() << " en PC=0x"
                  << std::hex << cur_pc << "\n";
        state = CPUState::ERROR;
        return false;
    }

    if (pc == cur_pc) {
        std::cout << "[FIN] Bucle infinito detectado (Programa terminado).\n";
        state = CPUState::HALTED;
        return false;
    }

    return (state == CPUState::RUNNING);
}

void CPU::run() {
    uint32_t limit = 1000000;
    uint32_t count = 0;

    while (step() && count < limit) {
        count++;
    }

    if (count >= limit) {
        std::cout << "\n[AVISO] Se alcanzo el limite de " << std::dec << limit << " instrucciones.\n";
        std::cout << "El programa probablemente termino y entro en un bucle infinito de seguridad.\n";
        state = CPUState::HALTED;
    }
}

// ── R-TYPE ──
// add, sub, sll, slt, sltu, xor, srl, sra, or, and
void CPU::execR(const Instruction& i) {
    uint32_t a = regs[i.rs1];
    uint32_t b = regs[i.rs2];
    uint32_t result = 0;

    switch (i.funct3) {
        case 0x0: // ADD / SUB
            result = (i.funct7 == 0x20) ? (a - b) : (a + b);
            break;
        case 0x1: // SLL
            result = a << (b & 0x1F);
            break;
        case 0x2: // SLT (signed)
            result = ((int32_t)a < (int32_t)b) ? 1 : 0;
            break;
        case 0x3: // SLTU (unsigned)
            result = (a < b) ? 1 : 0;
            break;
        case 0x4: // XOR
            result = a ^ b;
            break;
        case 0x5: // SRL / SRA
            if (i.funct7 == 0x20)
                result = (uint32_t)((int32_t)a >> (b & 0x1F));
            else
                result = a >> (b & 0x1F);
            break;
        case 0x6: // OR
            result = a | b;
            break;
        case 0x7: // AND
            result = a & b;
            break;
    }
    writeReg(i.rd, result);
}

// ── I-TYPE ───
// addi, slti, sltiu, xori, ori, andi, slli, srli, srai,
// lb, lh, lw, lbu, lhu, jalr, ecall
void CPU::execI(const Instruction& i, uint32_t cur_pc) {
    uint32_t rs1v = regs[i.rs1];

    // --- Loads ---
    if (i.opcode == 0x03) {
        uint32_t addr = (uint32_t)((int32_t)rs1v + i.imm);
        switch (i.funct3) {
            case 0x0: // LB  - sign extend byte
                writeReg(i.rd, (uint32_t)(int32_t)(int8_t)mem.readByte(addr));
                break;
            case 0x1: // LH  - sign extend halfword
                writeReg(i.rd, (uint32_t)(int32_t)(int16_t)mem.readHalf(addr));
                break;
            case 0x2: // LW
                writeReg(i.rd, mem.readWord(addr));
                break;
            case 0x4: // LBU - zero extend byte
                writeReg(i.rd, (uint32_t)mem.readByte(addr));
                break;
            case 0x5: // LHU - zero extend halfword
                writeReg(i.rd, (uint32_t)mem.readHalf(addr));
                break;
        }
        return;
    }

    // --- JALR ---
    if (i.opcode == 0x67) {
        uint32_t ret_addr = cur_pc + 4;
        uint32_t target   = (uint32_t)((int32_t)rs1v + i.imm) & ~1u;
        writeReg(i.rd, ret_addr);
        pc = target;
        return;
    }

    // --- ECALL / EBREAK ---
    if (i.opcode == 0x73) {
        if (i.imm == 0) handleEcall();
        else {
            std::cout << "[EBREAK] en PC=0x" << std::hex << cur_pc << "\n";
            state = CPUState::HALTED;
        }
        return;
    }

    // --- ALU immediate ---
    uint32_t result = 0;
    switch (i.funct3) {
        case 0x0: // ADDI
            result = (uint32_t)((int32_t)rs1v + i.imm);
            break;
        case 0x1: // SLLI
            result = rs1v << (i.imm & 0x1F);
            break;
        case 0x2: // SLTI (signed)
            result = ((int32_t)rs1v < i.imm) ? 1 : 0;
            break;
        case 0x3: // SLTIU (unsigned) - imm treated as unsigned after sign-extend
            result = (rs1v < (uint32_t)i.imm) ? 1 : 0;
            break;
        case 0x4: // XORI
            result = rs1v ^ (uint32_t)i.imm;
            break;
        case 0x5: // SRLI / SRAI
            if (i.funct7 == 0x20)
                result = (uint32_t)((int32_t)rs1v >> (i.imm & 0x1F)); // SRAI
            else
                result = rs1v >> (i.imm & 0x1F);                       // SRLI
            break;
        case 0x6: // ORI
            result = rs1v | (uint32_t)i.imm;
            break;
        case 0x7: // ANDI
            result = rs1v & (uint32_t)i.imm;
            break;
    }
    writeReg(i.rd, result);
}

// ── S-TYPE ──
// sb, sh, sw
void CPU::execS(const Instruction& i) {
    uint32_t addr = (uint32_t)((int32_t)regs[i.rs1] + i.imm);
    uint32_t val  = regs[i.rs2];
    switch (i.funct3) {
        case 0x0: mem.writeByte(addr, (uint8_t)val);  break; // SB
        case 0x1: mem.writeHalf(addr, (uint16_t)val); break; // SH
        case 0x2: mem.writeWord(addr, val);            break; // SW
    }
}

// ── B-TYPE ──
// beq, bne, blt, bge, bltu, bgeu
void CPU::execB(const Instruction& i, uint32_t cur_pc) {
    int32_t  sa = (int32_t)regs[i.rs1];
    int32_t  sb = (int32_t)regs[i.rs2];
    uint32_t ua = regs[i.rs1];
    uint32_t ub = regs[i.rs2];
    bool taken = false;

    switch (i.funct3) {
        case 0x0: taken = (sa == sb); break; // BEQ
        case 0x1: taken = (sa != sb); break; // BNE
        case 0x4: taken = (sa <  sb); break; // BLT
        case 0x5: taken = (sa >= sb); break; // BGE
        case 0x6: taken = (ua <  ub); break; // BLTU
        case 0x7: taken = (ua >= ub); break; // BGEU
    }

    pc = taken ? (uint32_t)((int32_t)cur_pc + i.imm)
               : (cur_pc + 4);
}

// ── U-TYPE ───
// lui, auipc
void CPU::execU(const Instruction& i, uint32_t cur_pc) {
    if (i.opcode == 0x37) {        // LUI
        writeReg(i.rd, (uint32_t)i.imm);
        pc = cur_pc + 4;
    } else if (i.opcode == 0x17) { // AUIPC
        writeReg(i.rd, (uint32_t)((int32_t)cur_pc + i.imm));
        pc = cur_pc + 4;
    }
}

// ── J-TYPE ───
// jal
void CPU::execJ(const Instruction& i, uint32_t cur_pc) {
    writeReg(i.rd, cur_pc + 4);                        // rd = return address
    pc = (uint32_t)((int32_t)cur_pc + i.imm);          // jump
}

// ── ECALL (SPIM-compatible syscalls) ──
void CPU::handleEcall() {
    uint32_t num = regs[17]; // a7 holds syscall number
    switch (num) {
        case 1:  // print_int
            std::cout << (int32_t)regs[10];
            break;
        case 4:  // print_string (a0 = address)
            {
                uint32_t addr = regs[10];
                while (true) {
                    uint8_t c = mem.readByte(addr++);
                    if (c == 0) break;
                    std::cout << (char)c;
                }
            }
            break;
        case 5:
            {
                int32_t v;
                std::cin >> v;
                writeReg(10, (uint32_t)v);
            }
            break;
        case 10: // exit
            std::cout << "\n[ecall exit] Programa terminado.\n";
            state = CPUState::HALTED;
            break;
        case 11: // print_char
            std::cout << (char)(regs[10] & 0xFF);
            break;
        case 17: // exit2(a0)
            std::cout << "\n[ecall exit2] Codigo: " << regs[10] << "\n";
            state = CPUState::HALTED;
            break;
        case 34: // print_hex
            std::cout << "0x" << std::hex << regs[10] << std::dec;
            break;
        default:
            std::cout << "[ecall " << num << " no implementado]\n";
            break;
    }
}

// ── Register inspection ──

void CPU::printReg(int r) const {
    std::cout << "x" << std::dec << std::setw(2) << r
              << " (" << std::setw(4) << std::left << regName(r) << std::right
              << ") = 0x" << std::hex << std::setfill('0') << std::setw(8)
              << regs[r] << "  (" << std::dec << (int32_t)regs[r] << ")\n";
}

void CPU::printAllRegs() const {
    std::cout << std::hex << std::setfill('0');
    std::cout << "  pc       = 0x" << std::setw(8) << pc << "\n";
    std::cout << std::dec;
    for (int i = 0; i < 32; i++) {
        printReg(i);
    }
}

void CPU::printRegs(const std::string& list) const {
    std::istringstream ss(list);
    std::string token;
    while (ss >> token) {
        int r = parseRegister(token);
        if (r >= 0) printReg(r);
        else std::cerr << "Registro desconocido: '" << token << "'\n";
    }
}
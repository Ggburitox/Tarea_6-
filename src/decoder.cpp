#include "decoder.h"

static uint32_t bits(uint32_t val, int hi, int lo) {
    return (val >> lo) & ((1u << (hi - lo + 1)) - 1);
}

static int32_t sext(uint32_t val, int n) {
    int shift = 32 - n;
    return (int32_t)(val << shift) >> shift;
}

static int32_t immI(uint32_t r) { return sext(bits(r,31,20), 12); }
static int32_t immS(uint32_t r) { return sext((bits(r,31,25)<<5)|bits(r,11,7), 12); }
static int32_t immB(uint32_t r) {
    return sext((bits(r,31,31)<<12)|(bits(r,7,7)<<11)|(bits(r,30,25)<<5)|(bits(r,11,8)<<1), 13);
}
static int32_t immU(uint32_t r) { return (int32_t)(r & 0xFFFFF000); }
static int32_t immJ(uint32_t r) {
    return sext((bits(r,31,31)<<20)|(bits(r,19,12)<<12)|(bits(r,20,20)<<11)|(bits(r,30,21)<<1), 21);
}

Instruction decode(uint32_t raw) {
    Instruction i;
    i.raw    = raw;
    i.opcode = bits(raw, 6, 0);
    i.rd     = bits(raw, 11, 7);
    i.rs1    = bits(raw, 19, 15);
    i.rs2    = bits(raw, 24, 20);
    i.funct3 = bits(raw, 14, 12);
    i.funct7 = bits(raw, 31, 25);

    switch (i.opcode) {

        // ── R-TYPE ──
        case 0x33:
            i.type = InstrType::R;
            switch (i.funct3) {
                case 0x0: i.name = (i.funct7 == 0x20) ? "sub" : "add"; break;
                case 0x1: i.name = "sll";  break;
                case 0x2: i.name = "slt";  break;
                case 0x3: i.name = "sltu"; break;
                case 0x4: i.name = "xor";  break;
                case 0x5: i.name = (i.funct7 == 0x20) ? "sra" : "srl"; break;
                case 0x6: i.name = "or";   break;
                case 0x7: i.name = "and";  break;
                default:  i.name = "R?";   break;
            }
            break;

        // ── I-TYPE: Loads ──
        case 0x03:
            i.type = InstrType::I;
            i.imm  = immI(raw);
            switch (i.funct3) {
                case 0x0: i.name = "lb";  break;
                case 0x1: i.name = "lh";  break;
                case 0x2: i.name = "lw";  break;
                case 0x4: i.name = "lbu"; break;
                case 0x5: i.name = "lhu"; break;
                default:  i.name = "L?";  break;
            }
            break;

        // ── I-TYPE: ALU immediate ─
        case 0x13:
            i.type = InstrType::I;
            i.imm  = immI(raw);
            switch (i.funct3) {
                case 0x0: i.name = "addi";  break;
                case 0x1: i.name = "slli";
                          i.imm  = (int32_t)bits(raw, 24, 20); // shamt
                          break;
                case 0x2: i.name = "slti";  break;
                case 0x3: i.name = "sltiu"; break;
                case 0x4: i.name = "xori";  break;
                case 0x5:
                    i.imm  = (int32_t)bits(raw, 24, 20); // shamt
                    i.name = (i.funct7 == 0x20) ? "srai" : "srli";
                    break;
                case 0x6: i.name = "ori";   break;
                case 0x7: i.name = "andi";  break;
                default:  i.name = "I?";    break;
            }
            break;

        // ── I-TYPE: JALR ──
        case 0x67:
            i.type = InstrType::I;
            i.imm  = immI(raw);
            i.name = "jalr";
            break;

        // ── I-TYPE: ECALL/EBREAK ──
        case 0x73:
            i.type = InstrType::I;
            i.imm  = immI(raw);
            i.name = (i.imm == 0) ? "ecall" : "ebreak";
            break;

        // ── S-TYPE: Stores ──
        case 0x23:
            i.type = InstrType::S;
            i.imm  = immS(raw);
            switch (i.funct3) {
                case 0x0: i.name = "sb"; break;
                case 0x1: i.name = "sh"; break;
                case 0x2: i.name = "sw"; break;
                default:  i.name = "S?"; break;
            }
            break;

        // ── B-TYPE: Branches ──
        case 0x63:
            i.type = InstrType::B;
            i.imm  = immB(raw);
            switch (i.funct3) {
                case 0x0: i.name = "beq";  break;
                case 0x1: i.name = "bne";  break;
                case 0x4: i.name = "blt";  break;
                case 0x5: i.name = "bge";  break;
                case 0x6: i.name = "bltu"; break;
                case 0x7: i.name = "bgeu"; break;
                default:  i.name = "B?";   break;
            }
            break;

        // ── U-TYPE ──
        case 0x37:
            i.type = InstrType::U;
            i.imm  = immU(raw);
            i.name = "lui";
            break;

        case 0x17:
            i.type = InstrType::U;
            i.imm  = immU(raw);
            i.name = "auipc";
            break;

        // ── J-TYPE ──
        case 0x6F:
            i.type = InstrType::J;
            i.imm  = immJ(raw);
            i.name = "jal";
            break;

        default:
            i.type = InstrType::UNKNOWN;
            i.name = "???";
            break;
    }
    return i;
}

// ── Nombrando los registros ───
static const char* ABI_NAMES[32] = {
    "zero","ra","sp","gp","tp",
    "t0","t1","t2",
    "s0","s1",
    "a0","a1","a2","a3","a4","a5","a6","a7",
    "s2","s3","s4","s5","s6","s7","s8","s9","s10","s11",
    "t3","t4","t5","t6"
};

std::string regName(int r) {
    if (r >= 0 && r < 32) return ABI_NAMES[r];
    return "x?";
}

int parseRegister(const std::string& name) {
    // Handle "xN" format
    if (name.size() >= 2 && name[0] == 'x') {
        try {
            int n = std::stoi(name.substr(1));
            if (n >= 0 && n < 32) return n;
        } catch (...) {}
    }
    // Handle ABI names
    for (int i = 0; i < 32; i++) {
        if (name == ABI_NAMES[i]) return i;
    }
    return -1;
}

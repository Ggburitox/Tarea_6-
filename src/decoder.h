#pragma once
#include <cstdint>
#include <string>

enum class InstrType { R, I, S, B, U, J, UNKNOWN };

struct Instruction {
    InstrType type   = InstrType::UNKNOWN;
    uint32_t  raw    = 0;
    uint8_t   opcode = 0;
    uint8_t   rd     = 0;
    uint8_t   rs1    = 0;
    uint8_t   rs2    = 0;
    uint8_t   funct3 = 0;
    uint8_t   funct7 = 0;
    int32_t   imm    = 0;
    std::string name;
};

Instruction decode(uint32_t raw);
std::string regName(int r);
int parseRegister(const std::string& name);

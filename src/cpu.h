#pragma once
#include <cstdint>
#include <string>
#include "memory.h"
#include "decoder.h"

enum class CPUState { RUNNING, HALTED, ERROR };

class CPU {
public:
    explicit CPU(Memory& mem);

    void reset();
    bool step();          // Execute one instruction; returns false if stopped
    void run();           // Execute until halted

    uint32_t getPC()  const { return pc; }
    uint32_t getReg(int r) const;
    CPUState getState()    const { return state; }

    void printAllRegs() const;
    void printRegs(const std::string& list) const;
    void printReg(int r) const;

private:
    uint32_t  pc;
    uint32_t  regs[32];
    Memory&   mem;
    CPUState  state;

    void writeReg(int rd, uint32_t val);

    void execR(const Instruction& i);
    void execI(const Instruction& i, uint32_t cur_pc);
    void execS(const Instruction& i);
    void execB(const Instruction& i, uint32_t cur_pc);
    void execU(const Instruction& i, uint32_t cur_pc);
    void execJ(const Instruction& i, uint32_t cur_pc);

    void handleEcall();
};

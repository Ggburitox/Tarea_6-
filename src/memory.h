#pragma once
#include <cstdint>
#include <vector>
#include <string>

class Memory {
public:
    static constexpr uint32_t MEM_SIZE = 0x01000000; // 16 MB

    Memory();

    bool loadFromFile(const std::string& path);
    void reset();

    uint8_t  readByte(uint32_t addr) const;
    uint16_t readHalf(uint32_t addr) const;
    uint32_t readWord(uint32_t addr) const;

    void writeByte(uint32_t addr, uint8_t  val);
    void writeHalf(uint32_t addr, uint16_t val);
    void writeWord(uint32_t addr, uint32_t val);

    void dump(uint32_t start, uint32_t end) const;

private:
    std::vector<uint8_t> data;
    void checkBounds(uint32_t addr, uint32_t size) const;
};

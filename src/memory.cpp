#include "memory.h"
#include <fstream>
#include <iostream>
#include <iomanip>
#include <stdexcept>

Memory::Memory() : data(MEM_SIZE, 0) {}

void Memory::reset() {
    std::fill(data.begin(), data.end(), 0);
}

bool Memory::loadFromFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "Error: No se pudo abrir '" << path << "'\n";
        return false;
    }
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    if ((uint32_t)size > MEM_SIZE) {
        std::cerr << "Error: Programa demasiado grande (" << size << " bytes)\n";
        return false;
    }
    std::fill(data.begin(), data.end(), 0);
    file.read(reinterpret_cast<char*>(data.data()), size);
    std::cout << "\"" << path << "\" cargado en memoria (" << size << " bytes)\n";
    return true;
}

void Memory::checkBounds(uint32_t addr, uint32_t size) const {
    if ((uint64_t) addr + size > MEM_SIZE) {
        std::ostringstream oss;
        oss << "Acceso fuera de rango: addr=0x" << std::hex << addr;
        throw std::out_of_range(oss.str());
    }
}

// Little endian read
uint8_t Memory::readByte(uint32_t addr) const {
    checkBounds(addr, 1);
    return data[addr];
}

uint16_t Memory::readHalf(uint32_t addr) const {
    checkBounds(addr, 2);
    return (uint16_t)data[addr] |
           ((uint16_t)data[addr + 1] << 8);
}

uint32_t Memory::readWord(uint32_t addr) const {
    checkBounds(addr, 4);
    return (uint32_t)data[addr]            |
           ((uint32_t)data[addr + 1] << 8)  |
           ((uint32_t)data[addr + 2] << 16) |
           ((uint32_t)data[addr + 3] << 24);
}

// Little Endian Write
void Memory::writeByte(uint32_t addr, uint8_t val) {
    checkBounds(addr, 1);
    data[addr] = val;
}

void Memory::writeHalf(uint32_t addr, uint16_t val) {
    checkBounds(addr, 2);
    data[addr]     = (uint8_t)(val & 0xFF);
    data[addr + 1] = (uint8_t)((val >> 8) & 0xFF);
}

void Memory::writeWord(uint32_t addr, uint32_t val) {
    checkBounds(addr, 4);
    data[addr]     = (uint8_t)(val & 0xFF);
    data[addr + 1] = (uint8_t)((val >> 8)  & 0xFF);
    data[addr + 2] = (uint8_t)((val >> 16) & 0xFF);
    data[addr + 3] = (uint8_t)((val >> 24) & 0xFF);
}

void Memory::dump(uint32_t start, uint32_t end) const {
    std::cout << std::hex << std::uppercase << std::setfill('0');
    std::cout << "Memoria [0x" << std::setw(8) << start
              << " - 0x" << std::setw(8) << end << "]:\n";
    for (uint32_t addr = start; addr <= end; addr++) {
        if ((addr - start) % 16 == 0)
            std::cout << "  0x" << std::setw(8) << addr << ": ";
        std::cout << std::setw(2) << (unsigned)data[addr] << " ";
        if ((addr - start) % 16 == 15)
            std::cout << "\n";
    }
    std::cout << "\n" << std::dec;
}

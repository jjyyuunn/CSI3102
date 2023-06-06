#ifndef __MEMORY_HPP__
#define __MEMORY_HPP__

#include <bitset>
#include <cassert>
#include <cstdio>

#define MEMORY_SIZE (32 * 1024 * 1024) // <-- 32-MB memory

class Memory {
  public:
    enum Endianness { LittleEndian, BigEndian };
    Memory(
      const Endianness endianness,
      const char *initFileName = nullptr
    ) : m_endianness(endianness), m_memory(new std::bitset<8>[MEMORY_SIZE]) {
      if (initFileName != nullptr) {
        // Each line of the memory initialization file consists of:
        //   1) the starting memory address of a 32-bit data in hexadecimal value
        //   2) the eight-digit hexadecimal value of the data
        // For example, "1000 ABCD1234" stores 0xABCD1234 to memory addresses from 0x1000 to 0x1003.
        FILE *initFile = fopen(initFileName, "r");
        assert(initFile != NULL);
        std::uint32_t addr, value;
        while (fscanf(initFile, " %x %x", &addr, &value) == 2) {
          printf("INFO: memory[0x%08lx..0x%08lx] <-- 0x%08lx\n",
                 (unsigned long)addr, (unsigned long)(addr + 3),
                 (unsigned long)value);
          if (m_endianness == LittleEndian) {
            m_memory[addr + 0] = (std::uint8_t)(value % 0x100); value >>= 8;
            m_memory[addr + 1] = (std::uint8_t)(value % 0x100); value >>= 8;
            m_memory[addr + 2] = (std::uint8_t)(value % 0x100); value >>= 8;
            m_memory[addr + 3] = (std::uint8_t)(value % 0x100);
          } else { // m_endianness == BigEndian
            m_memory[addr + 3] = (std::uint8_t)(value % 0x100); value >>= 8;
            m_memory[addr + 2] = (std::uint8_t)(value % 0x100); value >>= 8;
            m_memory[addr + 1] = (std::uint8_t)(value % 0x100); value >>= 8;
            m_memory[addr + 0] = (std::uint8_t)(value % 0x100);
          }
        }
        fclose(initFile);
      }
    }
    void printMemory() {
      for (size_t i = 0; i < MEMORY_SIZE; i += 4) {
        if (m_memory[i].any() || m_memory[i + 1].any() || m_memory[i + 2].any()
            || m_memory[i + 3].any()) {
          std::uint32_t value = 0;
          if (m_endianness == LittleEndian) {
            value += (m_memory[i + 3].to_ulong()); value <<= 8;
            value += (m_memory[i + 2].to_ulong()); value <<= 8;
            value += (m_memory[i + 1].to_ulong()); value <<= 8;
            value += (m_memory[i + 0].to_ulong());
          } else { // m_endianness == BigEndian
            value += (m_memory[i + 0].to_ulong()); value <<= 8;
            value += (m_memory[i + 1].to_ulong()); value <<= 8;
            value += (m_memory[i + 2].to_ulong()); value <<= 8;
            value += (m_memory[i + 3].to_ulong());
          }
          printf("  memory[0x%08x..0x%08x] = 0x%08lx\n", i, i + 3,
                 (unsigned long)value);
        }
      }
    }
    void access(
      const std::bitset<32> *address, const std::bitset<32> *writeData,
      const std::bitset<1> *memRead, const std::bitset<1> *memWrite,
      std::bitset<32> *readData
    );
    ~Memory() {
      delete[] m_memory;
    }
  private:
    // memory
    Endianness m_endianness;
    std::bitset<8> *m_memory;
};

#endif


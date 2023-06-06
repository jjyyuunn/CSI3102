#ifndef __REGISTER_FILE_HPP__
#define __REGISTER_FILE_HPP__

#include <bitset>
#include <cassert>
#include <cstdio>

class RegisterFile {
  public:
    RegisterFile(const char *initFileName = nullptr) {
      for (size_t i = 0; i < 32; i++) {
        m_registers[i].reset();
      }
      if (initFileName != nullptr) {
        // Each line of the memory initialization file consists of:
        //   1) the target register index in decimal value
        //   2) the eight-digit hexadecimal value of the data
        // For example, "10 ABCD1234" stores 0xABCD1234 to register #10.
        FILE *initFile = fopen(initFileName, "r");
        assert(initFile != NULL);
        std::uint32_t reg, val;
        while (fscanf(initFile, " %u %x", &reg, &val) == 2) {
          if (reg != 0) {
            printf("INFO: $%02u <-- 0x%08lx\n", reg, (unsigned long)val);
            m_registers[reg] = val;
          }
        }
      }
    }
    void printRegisters() {
      for (size_t i = 0; i < 32; i++) {
        if (m_registers[i].any()) {
          printf("  $%02u = 0x%08lx\n", i, m_registers[i].to_ulong());
        }
      }
    }
    void access(
      const std::bitset<5> *readRegister1, const std::bitset<5> *readRegister2,
      const std::bitset<5> *writeRegister, const std::bitset<32> *writeData,
      const std::bitset<1> *regWrite,
      std::bitset<32> *readData1, std::bitset<32> *readData2
    );
  private:
    // registers
    std::bitset<32> m_registers[32];
};

#endif


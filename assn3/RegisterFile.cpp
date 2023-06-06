#include "RegisterFile.hpp"

void RegisterFile::access(
  const std::bitset<5> *readRegister1, const std::bitset<5> *readRegister2,
  const std::bitset<5> *writeRegister, const std::bitset<32> *writeData,
  const std::bitset<1> *regWrite,
  std::bitset<32> *readData1, std::bitset<32> *readData2
) {
  if (regWrite != nullptr && regWrite->all()) {
    assert(writeRegister != nullptr && writeData != nullptr);
    if (*writeRegister != 0) { // prevent $0 (= $zero) from being written
      m_registers[writeRegister->to_ulong()] = (*writeData);
    }
  }
  if (readRegister1 != nullptr && readRegister2 != nullptr) {
    assert(readData1 != nullptr && readData2 != nullptr);
    (*readData1) = m_registers[readRegister1->to_ulong()];
    (*readData2) = m_registers[readRegister2->to_ulong()];
  }
}


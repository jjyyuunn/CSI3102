#include "Memory.hpp"

void Memory::access(
  const std::bitset<32> *address, const std::bitset<32> *writeData,
  const std::bitset<1> *memRead, const std::bitset<1> *memWrite,
  std::bitset<32> *readData
) {
  assert(address != nullptr);
  assert(memRead != nullptr);
  assert(readData != nullptr);
  if (memRead->all() && (memWrite != nullptr && memWrite->all())) {
    printf("ERROR: Both `memRead' and `memWrite' are set.\n");
    assert(!(memRead->all() && memWrite->all()));
  } else if (memRead->all()) {
    assert(address != nullptr);
    assert(readData != nullptr);
    std::uint32_t value = 0;
    if (m_endianness == LittleEndian) {
      value += (m_memory[address->to_ulong() + 3].to_ulong()); value <<= 8;
      value += (m_memory[address->to_ulong() + 2].to_ulong()); value <<= 8;
      value += (m_memory[address->to_ulong() + 1].to_ulong()); value <<= 8;
      value += (m_memory[address->to_ulong() + 0].to_ulong());
    } else { // m_endianness == BigEndian
      value += (m_memory[address->to_ulong() + 0].to_ulong()); value <<= 8;
      value += (m_memory[address->to_ulong() + 1].to_ulong()); value <<= 8;
      value += (m_memory[address->to_ulong() + 2].to_ulong()); value <<= 8;
      value += (m_memory[address->to_ulong() + 3].to_ulong());
    }
    (*readData) = value;
  } else if (memWrite != nullptr && memWrite->all()) { 
    assert(address != nullptr);
    assert(writeData != nullptr);
    std::uint32_t value = writeData->to_ulong();
    if (m_endianness == LittleEndian) {
      m_memory[address->to_ulong() + 0] = (std::uint8_t)(value % 0x100); value >>= 8;
      m_memory[address->to_ulong() + 1] = (std::uint8_t)(value % 0x100); value >>= 8;
      m_memory[address->to_ulong() + 2] = (std::uint8_t)(value % 0x100); value >>= 8;
      m_memory[address->to_ulong() + 3] = (std::uint8_t)(value % 0x100);
    } else { // m_endianness == BigEndian
      m_memory[address->to_ulong() + 3] = (std::uint8_t)(value % 0x100); value >>= 8;
      m_memory[address->to_ulong() + 2] = (std::uint8_t)(value % 0x100); value >>= 8;
      m_memory[address->to_ulong() + 1] = (std::uint8_t)(value % 0x100); value >>= 8;
      m_memory[address->to_ulong() + 0] = (std::uint8_t)(value % 0x100);
    }
  }
}


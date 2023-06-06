#ifndef __CPU_HPP__
#define __CPU_HPP__

#include "Memory.hpp"
#include "RegisterFile.hpp"

#include <cstdio>

class CPU {
  public:
    CPU(
      const std::uint32_t initialPC, const char *regFileName,
      const char *instMemFileName, const char *dataMemFileName
    ) : m_PC(initialPC),
        m_registerFile(new RegisterFile(regFileName)),
        m_instMemory(new Memory(Memory::LittleEndian, instMemFileName)),
        m_dataMemory(new Memory(Memory::LittleEndian, dataMemFileName)),
        m_currCycle(0) { }
    void printPVS() {
      printf("==================== Cycle %llu ====================\n", m_currCycle);
      printf("PC = 0x%08lx\n", m_PC.to_ulong());
      printf("Registers:\n");
      m_registerFile->printRegisters();
      printf("Data Memory:\n");
      m_dataMemory->printMemory();
      printf("Instruction Memory:\n");
      m_instMemory->printMemory();
    }
    virtual void advanceCycle() {
      m_currCycle++;
      printf("INFO: Simulating cycle %llu\n", m_currCycle);
    }
  protected:
    std::bitset<32> m_PC; // the Program Counter (PC) register
    // sequential circuits
    RegisterFile *m_registerFile; // the Register File (Registers)
    Memory *m_instMemory; // the Instruction Memory
    Memory *m_dataMemory; // the Data Memory
  private:
    // misc.
    unsigned long long m_currCycle; // <-- tracks the # of cycles the CPU processed
  public:
    /******************************************************/
    /* CPU::AND                                           */
    /*   - Perform logical AND on two BitWidth-bit values */
    /*   - output = input0 & input1;                      */
    /******************************************************/
    template<size_t BitWidth>
    void AND(
      const std::bitset<BitWidth> *input0, const std::bitset<BitWidth> *input1,
      std::bitset<BitWidth> *output
    ) {
      output->reset();
      for (size_t i = 0; i < BitWidth; i++) {
        output->set(i, input0->test(i) && input1->test(i));
      }
    }
    /********************************************/
    /* CPU::Add                                 */
    /*   - Add two BitWidth-bit signed integers */
    /*   - output = input0 + input1;            */
    /********************************************/
    template<size_t BitWidth>
    void Add(
      const std::bitset<BitWidth> *input0, const std::bitset<BitWidth> *input1,
      std::bitset<BitWidth> *output
    ) {
      output->reset();
      unsigned carry = 0;
      for (size_t i = 0; i < BitWidth; i++) {
        if (input0->test(i)) { carry++; }
        if (input1->test(i)) { carry++; }
        output->set(i, carry % 2);
        carry /= 2;
      }
    }
    /***********************************************************************/
    /* CPU::Mux                                                            */
    /*   - Select one of the two BitWidth-bit inputs as the output         */
    /*   - if (select == 0) { output = input0; } else { output = input1; } */
    /***********************************************************************/
    template<size_t BitWidth>
    void Mux(
      const std::bitset<BitWidth> *input0, const std::bitset<BitWidth> *input1,
      const std::bitset<1> *select,
      std::bitset<BitWidth> *output
    ) {
      (*output) = (select->none()) ? (*input0) : (*input1);
    }
    /***************************************************************************/
    /* CPU::SignExtend                                                         */
    /*   - Expand an InputBitWidth-bit signed integer to an OutputBitWidth-bit */
    /*     signed integer                                                      */
    /*   - output = input;                                                     */
    /***************************************************************************/
    template<size_t InputBitWidth, size_t OutputBitWidth>
    void SignExtend(
      const std::bitset<InputBitWidth> *input,
      std::bitset<OutputBitWidth> *output
    ) {
      output->reset();
      for (size_t i = 0; i < InputBitWidth - 1; i++) {
        output->set(i, input->test(i));
      }
      for (size_t i = InputBitWidth - 1; i < OutputBitWidth; i++) {
        output->set(i, input->test(InputBitWidth - 1));
      }
    }
    /*******************************************************************/
    /* CPU::ShiftLeft2                                                 */
    /*   - Shift the BitWidth-bit signed integer to the left by 2 bits */
    /*   - output = (input << 2);                                      */
    /*******************************************************************/
    template<size_t BitWidth>
    void ShiftLeft2(
      const std::bitset<BitWidth> *input,
      std::bitset<BitWidth> *output
    ) {
      output->reset();
      for (size_t i = 2; i < BitWidth - 1; i++) {
        output->set(i, input->test(i - 2));
      }
      output->set(BitWidth - 1, input->test(BitWidth - 1));
    }
    /*****************************************************************/
    /* CPU::ALU                                                      */
    /*   - Perform an arithmetic/logical operation on the two inputs */
    /*   - Refer to the slides and textbook for the exact operation  */
    /*****************************************************************/
    void ALU(
      const std::bitset<32> *input0, const std::bitset<32> *input1,
      const std::bitset<4> *control,
      std::bitset<32> *output, std::bitset<1> *zero
    ) {
      switch (control->to_ulong()) {
        case 0: { // and
          for (size_t i = 0; i < 32; i++) {
            output->set(i, input0->test(i) && input1->test(i));
          }
          break;
        }
        case 1: { // or
          for (size_t i = 0; i < 32; i++) {
            output->set(i, input0->test(i) || input1->test(i));
          }
          break;
        }
        case 2: { // add
          Add<32>(input0, input1, output);
          break;
        }
        case 6: { // sub
          // apply inversion-and-add-one to input1
          std::bitset<32> tmp0(0), tmp1(1), tmp2(0);
          for (size_t i = 0; i < 32; i++) {
            if (input1->test(i)) { tmp0.set(i, 0); }
            else { tmp0.set(i, 1); }
          }
          Add<32>(&tmp0, &tmp1, &tmp2);
    
          // add two values
          Add<32>(input0, &tmp2, output);
    
          break;
        }
        case 7: { // set on less than
          output->reset();
          if (!input0->test(31) && !input1->test(31)) {
            // positive <? positive
            output->set(0, input0->to_ullong() < input1->to_ullong());
          } else if (input0->test(31) && input1->test(31)) {
            // negative <? negative
            output->set(0, input0->to_ullong() >= input1->to_ullong());
          } else if (input0->test(31) && !input1->test(31)) {
            // negative <? positive
            output->set(0, true);
          } else {
            // positive <? negative
            output->set(0, false);
          }
    
          break;
        }
        case 12: { // nor
          for (size_t i = 0; i < 32; i++) {
            output->set(i, !(input0->test(i) || input1->test(i)));
          }
          break;
        }
        default: {
          printf("WARNING: Unsupported `control' 0x%02lx\n", control->to_ulong());
          fflush(stdout);
          break;
        }
      }
    
      zero->reset();
      zero->set(0, output->none());
    }
    /*****************************************************************************************/
    /* CPU::Control                                                                          */
    /*   - Produce appropriate control signals for the datapath w.r.t. the provided `opcode' */
    /*****************************************************************************************/
    void Control(
      const std::bitset<6> *opcode,
      std::bitset<1> *regDst, std::bitset<1> *branch, std::bitset<1> *memRead,
      std::bitset<1> *memToReg, std::bitset<2> *aluOp, std::bitset<1> *memWrite,
      std::bitset<1> *aluSrc, std::bitset<1> *regWrite
    ) {
      switch (opcode->to_ulong()) {
        case 0x23: { // lw
          (*regDst) = 0;
          (*branch) = 0;
          (*memRead) = 1;
          (*memToReg) = 1;
          (*aluOp) = 0;
          (*memWrite) = 0;
          (*aluSrc) = 1;
          (*regWrite) = 1;
    
          break;
        }
        case 0x2B: { // sw
          (*regDst) = 0;
          (*branch) = 0;
          (*memRead) = 0;
          (*memToReg) = 0;
          (*aluOp) = 0;
          (*memWrite) = 1;
          (*aluSrc) = 1;
          (*regWrite) = 0;
    
          break;
        }
        case 0x04: { // beq
          (*regDst) = 0;
          (*branch) = 1;
          (*memRead) = 0;
          (*memToReg) = 0;
          (*aluOp) = 1;
          (*memWrite) = 0;
          (*aluSrc) = 0;
          (*regWrite) = 0;
    
          break;
        }
        case 0x00: { // R-type instructions; add, sub, and, or, slt
          (*regDst) = 1;
          (*branch) = 0;
          (*memRead) = 0;
          (*memToReg) = 0;
          (*aluOp) = 2;
          (*memWrite) = 0;
          (*aluSrc) = 0;
          (*regWrite) = 1;
    
          break;
        }
        case 0x08: { // addi
          (*regDst) = 0;
          (*branch) = 0;
          (*memRead) = 0;
          (*memToReg) = 0;
          (*aluOp) = 0;
          (*memWrite) = 0;
          (*aluSrc) = 1;
          (*regWrite) = 1;

          break;
        }
        default: {
          printf("ERROR: Unsupported `opcode' %lu\n", opcode->to_ulong());
          assert(0);
          break;
        }
      }
    }
    /***********************************************************************************************/
    /* CPU::ALUControl                                                                             */
    /*   - Produce the appropriate control signal for the ALU w.r.t. the given `ALUOp' and `funct' */
    /***********************************************************************************************/
    void ALUControl(
      const std::bitset<2> *aluOp, const std::bitset<6> *funct,
      std::bitset<4> *aluControl
    ) {
      switch (aluOp->to_ulong()) {
        case 0:
          (*aluControl) = 2;
          break;
        case 1:
          (*aluControl) = 6;
          break;
        case 2: {
          switch (funct->to_ulong()) {
            case 0x20:
              (*aluControl) = 2;
              break;
            case 0x22:
              (*aluControl) = 6;
              break;
            case 0x24:
              (*aluControl) = 0;
              break;
            case 0x25:
              (*aluControl) = 1;
              break;
            case 0x2A:
              (*aluControl) = 7;
              break;
            default: {
              printf("WARNING: Unsupported `funct' %lu\n", funct->to_ulong());
              fflush(stdout);
              break;
            }
          }
          break;
        }
        default: {
          printf("WARNING: Unsupported `ALUOp' %lu\n", aluOp->to_ulong());
          fflush(stdout);
          break;
        }
      }
    }
};

#endif


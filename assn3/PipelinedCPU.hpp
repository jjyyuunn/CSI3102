#ifndef __PIPELINED_CPU_HPP__
#define __PIPELINED_CPU_HPP__

#include "CPU.hpp"

class PipelinedCPU : public CPU {
  public:
    PipelinedCPU(
      const std::uint32_t initialPC, const char *regFileName,
      const char *instMemFileName, const char *dataMemFileName,
      const bool enableDataForwarding = false,
      const bool enableHazardDetection = false,
      const bool enableIDBranch = false
    ) : CPU(initialPC, regFileName, instMemFileName, dataMemFileName),
        m_enableDataForwarding(enableDataForwarding),
        m_enableHazardDetection(enableHazardDetection),
        m_enableIDBranch(enableIDBranch) {
      // initialize the latches
      m_latch_IF_ID.pcPlus4.reset();
      m_latch_IF_ID.instr.reset();
      m_latch_ID_EX.ctrlEXALUSrc.reset();
      m_latch_ID_EX.ctrlEXALUOp.reset();
      m_latch_ID_EX.ctrlEXRegDst.reset();
      m_latch_ID_EX.ctrlMEMBranch.reset();
      m_latch_ID_EX.ctrlMEMMemRead.reset();
      m_latch_ID_EX.ctrlMEMMemWrite.reset();
      m_latch_ID_EX.ctrlWBRegWrite.reset();
      m_latch_ID_EX.ctrlWBMemToReg.reset();
      m_latch_EX_MEM.ctrlMEMBranch.reset();
      m_latch_EX_MEM.ctrlMEMMemRead.reset();
      m_latch_EX_MEM.ctrlMEMMemWrite.reset();
      m_latch_EX_MEM.ctrlWBRegWrite.reset();
      m_latch_EX_MEM.ctrlWBMemToReg.reset();
      m_latch_MEM_WB.ctrlWBRegWrite.reset();
      m_latch_MEM_WB.ctrlWBMemToReg.reset();
    }
  public:
    /******************************************************************/
    /* PipelinedCPU::advanceCycle                                     */
    /*   - Simulate a single clock cycle of the 5-stage pipelined CPU */
    /******************************************************************/
    virtual void advanceCycle() {
      CPU::advanceCycle();
      WriteBack();
      MemoryAccess();
      Execute();
      InstructionDecode();
      InstructionFetch();
    }
  private:
    // configuration parameters
    bool m_enableDataForwarding;
    bool m_enableHazardDetection;
    bool m_enableIDBranch;
    // latches
    struct {
      std::bitset<32> pcPlus4;  // PC+4
      std::bitset<32> instr;    // 32-bit instruction
    } m_latch_IF_ID; // IF-ID latch
    struct {
      std::bitset<32> pcPlus4;        // PC+4 (from IF) 0
      std::bitset<32> readData1;      // $rs 0
      std::bitset<32> readData2;      // $rt 0
      std::bitset<32> immediate;      // sign-extended immediate 0
      std::bitset<5> instr_20_16;     // instruction[20:16] 0
      std::bitset<5> instr_15_11;     // instruction[15:11] 0
      std::bitset<1> ctrlEXALUSrc;    // `ALUSrc' for EX
      std::bitset<2> ctrlEXALUOp;     // `ALUOp' for EX
      std::bitset<1> ctrlEXRegDst;    // `RegDst' for EX
      std::bitset<1> ctrlMEMBranch;   // `Branch' for MEM
      std::bitset<1> ctrlMEMMemRead;  // `MemRead' for MEM
      std::bitset<1> ctrlMEMMemWrite; // `MemWrite' for MEM
      std::bitset<1> ctrlWBRegWrite;  // `RegWrite' for WB
      std::bitset<1> ctrlWBMemToReg;  // `MemToReg' for WB
    } m_latch_ID_EX; // ID-EX latch
    struct {
      std::bitset<32> branchTarget;   // (PC+4)+(Immed<<2) 0
      std::bitset<32> aluResult;      // result from the ALU
      std::bitset<1> aluZero;         // zero from the ALU
      std::bitset<32> readData2;      // $rt (from ID) 0
      std::bitset<5> rd;              // rd 0
      std::bitset<1> ctrlMEMBranch;   // `Branch' for MEM 0
      std::bitset<1> ctrlMEMMemRead;  // `MemRead' for MEM 0
      std::bitset<1> ctrlMEMMemWrite; // `MemWrite' for MEM 0
      std::bitset<1> ctrlWBRegWrite;  // `RegWrite' for WB 0
      std::bitset<1> ctrlWBMemToReg;  // `MemToReg' for WB 0
    } m_latch_EX_MEM; // EX-MEM latch
    struct {
      std::bitset<32> readData;       // readData from the data memory 0
      std::bitset<32> aluResult;      // result from the ALU (from EX) 0
      std::bitset<5> rd;              // rd (from EX) 0
      std::bitset<1> ctrlWBRegWrite;  // `RegWrite' for WB
      std::bitset<1> ctrlWBMemToReg;  // `MemToReg' for WB
    } m_latch_MEM_WB; // MEM-WB latch
  public:
    virtual void printPVS() {
      CPU::printPVS();
      printf("Latches:\n");
      printf("  IF-ID Latch:\n");
      printf("    pcPlus4 = 0x%08lx\n", m_latch_IF_ID.pcPlus4.to_ulong());
      printf("    instr   = 0x%08lx\n", m_latch_IF_ID.instr.to_ulong());
      printf("  ID-EX Latch:\n");
      printf("    pcPlus4         = 0x%08lx\n", m_latch_ID_EX.pcPlus4.to_ulong());
      printf("    readData1       = 0x%08lx\n", m_latch_ID_EX.readData1.to_ulong());
      printf("    readData2       = 0x%08lx\n", m_latch_ID_EX.readData2.to_ulong());
      printf("    immediate       = 0x%08lx\n", m_latch_ID_EX.immediate.to_ulong());
      printf("    instr_20_16     = 0b%s\n", m_latch_ID_EX.instr_20_16.to_string().c_str());
      printf("    instr_15_11     = 0b%s\n", m_latch_ID_EX.instr_15_11.to_string().c_str());
      printf("    ctrlEXALUSrc    = 0b%s\n", m_latch_ID_EX.ctrlEXALUSrc.to_string().c_str());
      printf("    ctrlEXALUOp     = 0b%s\n", m_latch_ID_EX.ctrlEXALUOp.to_string().c_str());
      printf("    ctrlEXRegDst    = 0b%s\n", m_latch_ID_EX.ctrlEXRegDst.to_string().c_str());
      printf("    ctrlMEMBranch   = 0b%s\n", m_latch_ID_EX.ctrlMEMBranch.to_string().c_str());
      printf("    ctrlMEMMemRead  = 0b%s\n", m_latch_ID_EX.ctrlMEMMemRead.to_string().c_str());
      printf("    ctrlMEMMemWrite = 0b%s\n", m_latch_ID_EX.ctrlMEMMemWrite.to_string().c_str());
      printf("    ctrlWBRegWrite  = 0b%s\n", m_latch_ID_EX.ctrlWBRegWrite.to_string().c_str());
      printf("    ctrlWBMemToReg  = 0b%s\n", m_latch_ID_EX.ctrlWBMemToReg.to_string().c_str());
      printf("  EX-MEM Latch:\n");
      printf("    branchTarget    = 0x%08lx\n", m_latch_EX_MEM.branchTarget.to_ulong());
      printf("    aluResult       = 0x%08lx\n", m_latch_EX_MEM.aluResult.to_ulong());
      printf("    aluZero         = 0b%s\n", m_latch_EX_MEM.aluZero.to_string().c_str());
      printf("    readData2       = 0x%08lx\n", m_latch_EX_MEM.readData2.to_ulong());
      printf("    rd              = 0b%s\n", m_latch_EX_MEM.rd.to_string().c_str());
      printf("    ctrlMEMBranch   = 0b%s\n", m_latch_EX_MEM.ctrlMEMBranch.to_string().c_str());
      printf("    ctrlMEMMemRead  = 0b%s\n", m_latch_EX_MEM.ctrlMEMMemRead.to_string().c_str());
      printf("    ctrlMEMMemWrite = 0b%s\n", m_latch_EX_MEM.ctrlMEMMemWrite.to_string().c_str());
      printf("    ctrlWBRegWrite  = 0b%s\n", m_latch_EX_MEM.ctrlWBRegWrite.to_string().c_str());
      printf("    ctrlWBMemToReg  = 0b%s\n", m_latch_EX_MEM.ctrlWBMemToReg.to_string().c_str());
      printf("  MEM-WB Latch:\n");
      printf("    readData       = 0x%08lx\n", m_latch_MEM_WB.readData.to_ulong());
      printf("    aluResult      = 0x%08lx\n", m_latch_MEM_WB.aluResult.to_ulong());
      printf("    rd             = 0b%s\n", m_latch_MEM_WB.rd.to_string().c_str());
      printf("    ctrlWBRegWrite = 0b%s\n", m_latch_MEM_WB.ctrlWBRegWrite.to_string().c_str());
      printf("    ctrlWBMemToReg = 0b%s\n", m_latch_MEM_WB.ctrlWBMemToReg.to_string().c_str());
      fflush(stdout);
    }
  private:
    // pipeline stages
    void InstructionFetch();
    void InstructionDecode();
    void Execute();
    void MemoryAccess();
    void WriteBack();
};

#endif


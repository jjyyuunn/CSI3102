#include "PipelinedCPU.hpp"

void PipelinedCPU::InstructionFetch() {
  std::bitset<32> PC;
  std::bitset<32> addFour(4);
  Add(&m_PC, &addFour, &m_latch_IF_ID.pcPlus4);
  m_PC = m_latch_IF_ID.pcPlus4;
  //m_latch_IF_ID.pcPlus4 = PC;


  std::bitset<32> writeData; std::bitset<1> memRead; std::bitset<1> memWrite; std::bitset<32> readData;
  memRead = 1;
  memWrite = 0;
  readData = 0;
  m_instMemory->access(&m_PC, &writeData, &memRead, &memWrite, &readData);
  m_latch_IF_ID.instr = readData;

  
  Add(&m_PC, &addFour, &m_latch_IF_ID.pcPlus4);
}

void PipelinedCPU::InstructionDecode() {
  std::bitset<6> opcode;

  //register
  std::bitset<5> readRegister1;
  std::bitset<32> writeData; std::bitset<32> readData1; std::bitset<32> readData2;

  m_latch_ID_EX.pcPlus4 = m_latch_IF_ID.pcPlus4; //pc

  std::string s;
  std::string str = m_latch_IF_ID.instr.to_string();
  s = str.substr(0,6);
  std::bitset<6> op(s); opcode = op; s = ""; //opcode

  s = str.substr(6,11);
  std::bitset<5> rs(s); readRegister1 = rs; s="";
  s = str.substr(11,16);
  std::bitset<5> rt(s); m_latch_ID_EX.instr_20_16 = rt; s="";
  s = str.substr(16,21);
  std::bitset<5> rd(s); m_latch_ID_EX.instr_15_11 = rd; s=""; //rs rt rd

  std::bitset<16> imm;
  s = str.substr(16,32);
  std::bitset<16> br(s); imm = br; s="";
  SignExtend(&imm, &m_latch_ID_EX.immediate); //sign extend

  m_registerFile->access(&readRegister1, &m_latch_ID_EX.instr_20_16, &m_latch_ID_EX.instr_15_11, 
  &writeData, &m_latch_ID_EX.ctrlWBRegWrite, &m_latch_ID_EX.readData1, &m_latch_ID_EX.readData2); //reg

  Control(&opcode, &m_latch_ID_EX.ctrlEXRegDst, &m_latch_ID_EX.ctrlMEMBranch, &m_latch_ID_EX.ctrlMEMMemRead, 
  &m_latch_ID_EX.ctrlWBMemToReg, &m_latch_ID_EX.ctrlEXALUOp, &m_latch_ID_EX.ctrlMEMMemWrite, 
  &m_latch_ID_EX.ctrlEXALUSrc, &m_latch_ID_EX.ctrlWBRegWrite);
}

void PipelinedCPU::Execute() {
  m_latch_EX_MEM.ctrlMEMBranch = m_latch_ID_EX.ctrlMEMBranch;
  m_latch_EX_MEM.ctrlMEMMemRead = m_latch_ID_EX.ctrlMEMMemRead;
  m_latch_EX_MEM.ctrlMEMMemWrite = m_latch_ID_EX.ctrlMEMMemWrite;
  m_latch_EX_MEM.ctrlWBRegWrite = m_latch_ID_EX.ctrlWBRegWrite;
  m_latch_EX_MEM.ctrlWBMemToReg = m_latch_ID_EX.ctrlWBMemToReg;

  Mux(&m_latch_ID_EX.instr_20_16, &m_latch_ID_EX.instr_15_11, &m_latch_ID_EX.ctrlEXRegDst, &m_latch_EX_MEM.rd);

  m_latch_EX_MEM.readData2 = m_latch_ID_EX.readData2;

  //PC(branch)
  std::bitset<32> ShiftEx;
  ShiftLeft2(&m_latch_ID_EX.immediate, &ShiftEx);
  Add(&m_latch_ID_EX.pcPlus4, &ShiftEx, &m_latch_EX_MEM.branchTarget);

  std::bitset<4> aluControl;
  std::string s;
  std::string str = m_latch_ID_EX.immediate.to_string();
  s = str.substr(26,32);
  std::bitset<6> funct(s);

  //ALU
  ALUControl(&m_latch_ID_EX.ctrlEXALUOp, &funct, &aluControl);

  std::bitset<32> inputData;
  Mux(&m_latch_ID_EX.readData2, &m_latch_ID_EX.immediate, &m_latch_ID_EX.ctrlEXALUSrc, &inputData);
  ALU(&m_latch_ID_EX.readData1, &inputData, &aluControl, &m_latch_EX_MEM.aluResult, &m_latch_EX_MEM.aluZero);
}

void PipelinedCPU::MemoryAccess() {
  m_dataMemory->access(&m_latch_EX_MEM.aluResult, &m_latch_EX_MEM.readData2, &m_latch_EX_MEM.ctrlMEMMemRead, 
  &m_latch_EX_MEM.ctrlMEMMemWrite, &m_latch_MEM_WB.readData);

  m_latch_MEM_WB.aluResult = m_latch_EX_MEM.aluResult;
  m_latch_MEM_WB.rd = m_latch_EX_MEM.rd;
  m_latch_MEM_WB.ctrlWBRegWrite = m_latch_EX_MEM.ctrlWBRegWrite;
  m_latch_MEM_WB.ctrlWBMemToReg = m_latch_EX_MEM.ctrlWBMemToReg;

  std::bitset<1> PCSrc;
  AND(&m_latch_EX_MEM.ctrlMEMBranch, &m_latch_EX_MEM.aluZero, &PCSrc);
  std::bitset<32> PC2;
  Mux(&m_PC, &m_latch_EX_MEM.branchTarget, &PCSrc, &PC2);
  m_PC = PC2;
}

void PipelinedCPU::WriteBack() {
  std::bitset<32> writeData;
  Mux(&m_latch_MEM_WB.aluResult, &m_latch_MEM_WB.readData, &m_latch_MEM_WB.ctrlWBMemToReg, &writeData);
  m_registerFile->access(0, 0, &m_latch_MEM_WB.rd, &writeData, &m_latch_MEM_WB.ctrlWBRegWrite, 0, 0);
}
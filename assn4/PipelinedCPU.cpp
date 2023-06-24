#include "PipelinedCPU.hpp"
#include "iostream"

void PipelinedCPU::InstructionFetch() {
  std::bitset<32> PC;
  std::bitset<32> addFour(4);

  if (m_HazDetUnit_to_IF_PCWrite==1){

    Add(&m_PC, &addFour, &PC);
    m_PC = PC;
    
    CPU::Mux(&m_PC, &m_MEM_to_IF_branchTarget, &m_MEM_to_IF_PCSrc, &m_PC);
    
    Add(&PC, &addFour, &m_latch_IF_ID.pcPlus4);
  }

  if (m_HazDetUnit_to_IF_IFIDWrite==1){
    std::bitset<32> writeData; std::bitset<1> memRead; std::bitset<1> memWrite; std::bitset<32> readData;
    memRead = 1;
    memWrite = 0;
    readData = 0;
    m_instMemory->access(&m_PC, &writeData, &memRead, &memWrite, &readData);
    m_latch_IF_ID.instr = readData;
  }
}

void PipelinedCPU::InstructionDecode() {
  std::bitset<6> opcode;

  //register
  std::bitset<32> writeData; std::bitset<32> readData1; std::bitset<32> readData2;

  m_latch_ID_EX.pcPlus4 = m_latch_IF_ID.pcPlus4;

  std::string s;
  std::string str = m_latch_IF_ID.instr.to_string();
  s = str.substr(0,6);
  std::bitset<6> op(s); opcode = op; s = ""; //opcode

  s = str.substr(6,11);
  std::bitset<5> rs(s); m_latch_ID_EX.instr_25_21 = rs; s="";
  s = str.substr(11,16);
  std::bitset<5> rt(s); m_latch_ID_EX.instr_20_16 = rt; s="";
  s = str.substr(16,21);
  std::bitset<5> rd(s); m_latch_ID_EX.instr_15_11 = rd; s=""; //rs rt rd

  std::bitset<16> imm;
  s = str.substr(16,32);
  std::bitset<16> br(s); imm = br; s="";
  SignExtend(&imm, &m_latch_ID_EX.immediate); //sign extend

  m_registerFile->access(&m_latch_ID_EX.instr_25_21, &m_latch_ID_EX.instr_20_16, nullptr, 
  nullptr, nullptr, &m_latch_ID_EX.readData1, &m_latch_ID_EX.readData2); //reg

  std::bitset<1> PCWrite; std::bitset<1> IFIDWrite; std::bitset<1> ctrlSelect;
  if(m_enableHazardDetection) {
    PipelinedCPU::HazardDetectionUnit(&rs, &rt, &m_EX_to_HazDetUnit_memRead, &m_EX_to_HazDetUnit_rt, &m_HazDetUnit_to_IF_PCWrite, 
          &m_HazDetUnit_to_IF_IFIDWrite, &ctrlSelect);
  }

  Control(&opcode, &m_latch_ID_EX.ctrlEXRegDst, &m_latch_ID_EX.ctrlMEMBranch, &m_latch_ID_EX.ctrlMEMMemRead, 
  &m_latch_ID_EX.ctrlWBMemToReg, &m_latch_ID_EX.ctrlEXALUOp, &m_latch_ID_EX.ctrlMEMMemWrite, 
  &m_latch_ID_EX.ctrlEXALUSrc, &m_latch_ID_EX.ctrlWBRegWrite);

  if (m_enableHazardDetection && ctrlSelect==1){
    m_latch_ID_EX.ctrlEXRegDst =0;
    m_latch_ID_EX.ctrlMEMBranch =0;
    m_latch_ID_EX.ctrlMEMMemRead =0;
    m_latch_ID_EX.ctrlWBMemToReg =0;
    m_latch_ID_EX.ctrlEXALUOp =0;
    m_latch_ID_EX.ctrlMEMMemWrite =0;
    m_latch_ID_EX.ctrlEXALUSrc =0;
    m_latch_ID_EX.ctrlWBRegWrite =0;
  }
}

void PipelinedCPU::Execute() {
  m_latch_EX_MEM.ctrlMEMBranch = m_latch_ID_EX.ctrlMEMBranch;
  m_latch_EX_MEM.ctrlMEMMemRead = m_latch_ID_EX.ctrlMEMMemRead;
  m_latch_EX_MEM.ctrlMEMMemWrite = m_latch_ID_EX.ctrlMEMMemWrite;
  m_latch_EX_MEM.ctrlWBRegWrite = m_latch_ID_EX.ctrlWBRegWrite;
  m_latch_EX_MEM.ctrlWBMemToReg = m_latch_ID_EX.ctrlWBMemToReg;

  m_EX_to_HazDetUnit_memRead = m_latch_ID_EX.ctrlMEMMemRead;
  m_EX_to_HazDetUnit_rt = m_latch_ID_EX.instr_20_16;

  //PC(branch)
  std::bitset<32> ShiftEx;
  ShiftLeft2(&m_latch_ID_EX.immediate, &ShiftEx);
  Add(&m_latch_ID_EX.pcPlus4, &ShiftEx, &m_latch_EX_MEM.branchTarget);

  CPU::Mux(&m_latch_ID_EX.instr_20_16, &m_latch_ID_EX.instr_15_11, &m_latch_ID_EX.ctrlEXRegDst, &m_latch_EX_MEM.rd);


  m_latch_EX_MEM.readData2 = m_latch_ID_EX.readData2;

  std::bitset<4> aluControl;
  std::string s;
  std::string str = m_latch_ID_EX.immediate.to_string();
  s = str.substr(26,32);
  std::bitset<6> funct(s);

  //ALU
  ALUControl(&m_latch_ID_EX.ctrlEXALUOp, &funct, &aluControl);

  if(m_enableDataForwarding){
    std::bitset<32> inputData1;
    std::bitset<2UL> forwardA; std::bitset<2UL> forwardB; std::bitset<32> output1; std::bitset<32> output2;
    PipelinedCPU::ForwardingUnit(&m_latch_ID_EX.instr_25_21, &m_latch_ID_EX.instr_20_16, &m_MEM_to_FwdUnit_regWrite, 
                                &m_MEM_to_FwdUnit_rd, &m_WB_to_FwdUnit_regWrite, &m_WB_to_FwdUnit_rd, &forwardA, &forwardB);

    PipelinedCPU::Mux(&m_latch_ID_EX.readData1, &m_WB_to_FwdUnit_rdValue, &m_MEM_to_FwdUnit_rdValue, &forwardA, &output1);
    PipelinedCPU::Mux(&m_latch_ID_EX.readData2, &m_WB_to_FwdUnit_rdValue, &m_MEM_to_FwdUnit_rdValue, &forwardB, &output2);
    m_latch_EX_MEM.readData2 = output2;

    CPU::Mux(&output2, &m_latch_ID_EX.immediate, &m_latch_ID_EX.ctrlEXALUSrc, &inputData1);
    ALU(&output1, &inputData1, &aluControl, &m_latch_EX_MEM.aluResult, &m_latch_EX_MEM.aluZero);
  }
  else {
    std::bitset<32> inputData2;
    CPU::Mux(&m_latch_ID_EX.readData2, &m_latch_ID_EX.immediate, &m_latch_ID_EX.ctrlEXALUSrc, &inputData2);
    ALU(&m_latch_ID_EX.readData1, &inputData2, &aluControl, &m_latch_EX_MEM.aluResult, &m_latch_EX_MEM.aluZero);
  }
}

void PipelinedCPU::MemoryAccess() {
  m_dataMemory->access(&m_latch_EX_MEM.aluResult, &m_latch_EX_MEM.readData2, &m_latch_EX_MEM.ctrlMEMMemRead, 
  &m_latch_EX_MEM.ctrlMEMMemWrite, &m_latch_MEM_WB.readData);

  m_latch_MEM_WB.aluResult = m_latch_EX_MEM.aluResult;
  m_latch_MEM_WB.rd = m_latch_EX_MEM.rd;
  m_latch_MEM_WB.ctrlWBRegWrite = m_latch_EX_MEM.ctrlWBRegWrite;
  m_latch_MEM_WB.ctrlWBMemToReg = m_latch_EX_MEM.ctrlWBMemToReg;

  AND(&m_latch_EX_MEM.ctrlMEMBranch, &m_latch_EX_MEM.aluZero, &m_MEM_to_IF_PCSrc);
  m_MEM_to_IF_branchTarget=m_latch_EX_MEM.branchTarget;

  //forwarding unit
  m_MEM_to_FwdUnit_regWrite = m_latch_EX_MEM.ctrlWBRegWrite;
  m_MEM_to_FwdUnit_rd = m_latch_EX_MEM.rd;
  m_MEM_to_FwdUnit_rdValue = m_latch_EX_MEM.aluResult;
}

void PipelinedCPU::WriteBack() {
  std::bitset<32> writeData;
  CPU::Mux(&m_latch_MEM_WB.aluResult, &m_latch_MEM_WB.readData, &m_latch_MEM_WB.ctrlWBMemToReg, &writeData);
  m_registerFile->access(nullptr, nullptr, &m_latch_MEM_WB.rd, &writeData, &m_latch_MEM_WB.ctrlWBRegWrite, nullptr, nullptr);

  //forwarding unit
  m_WB_to_FwdUnit_regWrite = m_latch_MEM_WB.ctrlWBRegWrite;
  m_WB_to_FwdUnit_rd = m_latch_MEM_WB.rd;
  m_WB_to_FwdUnit_rdValue = writeData;
}

void PipelinedCPU::ForwardingUnit(
  const std::bitset<5> *ID_EX_rs, const std::bitset<5> *ID_EX_rt,
  const std::bitset<1> *EX_MEM_regWrite, const std::bitset<5> *EX_MEM_rd,
  const std::bitset<1> *MEM_WB_regWrite, const std::bitset<5> *MEM_WB_rd,
  std::bitset<2> *forwardA, std::bitset<2> *forwardB
) {
  if((*EX_MEM_regWrite == 1) && *EX_MEM_rd != 0 && *EX_MEM_rd == *ID_EX_rs){
    *forwardA = 0b010;
  }
  else if (*MEM_WB_regWrite == 1 && MEM_WB_rd !=0 && ~(*EX_MEM_regWrite == 1 && (*EX_MEM_rd != 0) && 
            (*EX_MEM_rd != *ID_EX_rs)) && *MEM_WB_rd == *ID_EX_rs){
    *forwardA = 0b01;
  }
  else {
    *forwardA = 0b00;
  }

  if((*EX_MEM_regWrite == 1) && *EX_MEM_rd != 0 && *EX_MEM_rd == *ID_EX_rt){
    *forwardB = 0b10;
  }
  else if (*MEM_WB_regWrite == 1 && MEM_WB_rd !=0 && ~(*EX_MEM_regWrite == 1 && (*EX_MEM_rd != 0) && 
            (*EX_MEM_rd != *ID_EX_rt)) && *MEM_WB_rd == *ID_EX_rt){
    *forwardB = 0b01;
  }
  else {
    *forwardB = 0b00;
  }
}

void PipelinedCPU::HazardDetectionUnit(
  const std::bitset<5> *IF_ID_rs, const std::bitset<5> *IF_ID_rt,
  const std::bitset<1> *ID_EX_memRead, const std::bitset<5> *ID_EX_rt,
  std::bitset<1> *PCWrite, std::bitset<1> *IFIDWrite, std::bitset<1> *ctrlSelect
) {
  if (*ID_EX_memRead==1 && ((*ID_EX_rt==*IF_ID_rs)||(*ID_EX_rt==*IF_ID_rt))){
    *PCWrite = 0;
    *IFIDWrite =0;
    *ctrlSelect =1;
  }
  else{
    *PCWrite = 1;
    *IFIDWrite =1;
    *ctrlSelect =0;
  }
}
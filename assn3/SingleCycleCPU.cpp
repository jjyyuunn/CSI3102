#include "SingleCycleCPU.hpp"

/*****************************************************************/
/* SingleCycleCPU::advanceCycle                                  */
/*   - Execute a single MIPS instruction in a single clock cycle */
/*****************************************************************/
void SingleCycleCPU::advanceCycle() {
  /* DO NOT CHANGE THE FOLLOWING LINE */
  CPU::advanceCycle();

  std::bitset<6> opcode; std::bitset<1> regDst; std::bitset<1>branch; std::bitset<1> memRead;
  std::bitset<1> memToReg; std::bitset<2> aluOp; std::bitset<1> memWrite;
  std::bitset<1> aluSrc; std::bitset<1> regWrite; 

  //ALU control
  std::bitset<6> funct; std::bitset<4> aluControl; 

  //MUX
  std::bitset<1> select;

  //Branch Address
  std::bitset<16> branchAd; std::bitset<32> extended; std::bitset<32> finalAd;

  //Register
  std::bitset<5> readRegister1; std::bitset<5> readRegister2; std::bitset<5> writeRegister;
  std::bitset<32> writeData; std::bitset<32> readData1; std::bitset<32> readData2;

  //memory
  std::bitset<32> address; std::bitset<32> readData;

  //PC
  std::bitset<32> addFour; addFour = 4;
  std::bitset<32> PC; std::bitset<32> PCplus4; 
  memRead = 1;
  memWrite = 0;
  readData = 0;
  PC = m_PC;

  //IF
  m_instMemory->access(&PC, &writeData, &memRead, &memWrite, &readData);
  Add(&PC, &addFour, &PCplus4);

  //ID
  std::string s;
  std::string str = readData.to_string();
  s = str.substr(0,6);
  std::bitset<6> op(s); opcode = op; s = ""; //opcode

  s = str.substr(6,11);
  std::bitset<5> rs(s); readRegister1 = rs; s="";
  s = str.substr(11,16);
  std::bitset<5> rt(s); readRegister2 = rt; s="";
  s = str.substr(16,21);
  std::bitset<5> rd(s); writeRegister = rd; s=""; //rs rt rd

  s = str.substr(26,32);
  std::bitset<6> fu(s); funct = fu; s=""; //funct

  s = str.substr(16,32);
  std::bitset<16> br(s); branchAd = br; s=""; //sign extend
  SignExtend(&branchAd, &extended);

  //writeData에 초기값을 줘라 -> nullptrs
  m_registerFile->access(&readRegister1, &readRegister2, &writeRegister, &writeData, &regWrite, &readData1, &readData2); //rs, rt 반환

  Control(&opcode, &regDst, &branch, &memRead, &memToReg, &aluOp, &memWrite, &aluSrc, &regWrite);
  ALUControl(&aluOp, &funct, &aluControl);

  std::bitset<5> writeRegisterFinal;
  Mux(&readRegister2, &writeRegister, &regDst, &writeRegisterFinal);

  //m_registerFile->access(readRegister1, readRegister2, writeRegisterFinal, writeData, regWrite, readData1, readData2);

  //EX
  std::bitset<32> ShiftEx;
  ShiftLeft2(&extended, &ShiftEx);
  Add(&PCplus4, &ShiftEx, &finalAd); //For PC
  
  std::bitset<32> inputData;
  Mux(&readData2, &extended, &aluSrc, &inputData);

  std::bitset<32> ALUOutput; std::bitset<1> zero;
  ALU(&readData1, &inputData, &aluControl, &ALUOutput, &zero); //ALU

  //MEM
  m_dataMemory->access(&ALUOutput, &readData2, &memRead, &memWrite, &readData);

  //WB
  Mux(&ALUOutput, &readData, &memToReg, &writeData);
  m_registerFile->access(&readRegister1, &readRegister2, &writeRegisterFinal, &writeData, &regWrite, &readData1, &readData2); //write to reg

  AND(&branch, &zero, &select);
  std::bitset<32> finalPC;
  Mux(&PCplus4, &finalAd, &select, &finalPC);
  m_PC = finalPC;
}
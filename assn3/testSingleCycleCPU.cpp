#include "SingleCycleCPU.hpp"

#include <cstdio>
#include <cstdlib>

int main(int argc, char **argv) {
  if (argc != 6) {
    fprintf(stderr, "Usage: %s initialPC regFileName instMemFileName dataMemFileName numCycles\n", argv[0]);
    fflush(stdout);
    exit(-1);
  }

  const std::uint32_t initialPC = (std::uint32_t)atoll(argv[1]);
  const char *regFileName = argv[2];
  const char *instMemFileName = argv[3];
  const char *dataMemFileName = argv[4];
  const std::uint64_t numCycles = (std::uint64_t)atoll(argv[5]);

  SingleCycleCPU *cpu = new SingleCycleCPU(initialPC, regFileName, instMemFileName, dataMemFileName);

  std::bitset<32> input0(0x11111111), input1(0x00101010), output0, output1;
  cpu->ShiftLeft2<32>(&input1, &output0);
  assert(output0.to_ulong() == 0x00404040);

  cpu->printPVS();
  for (size_t i = 0; i < numCycles; i++) {
    cpu->advanceCycle();
    cpu->printPVS();
  }

  delete cpu;

  return 0;
}


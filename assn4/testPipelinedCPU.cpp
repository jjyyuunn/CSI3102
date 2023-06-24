#include "PipelinedCPU.hpp"

#include <cstdio>
#include <cstdlib>

int main(int argc, char **argv) {
  if (argc != 8) {
    fprintf(stderr, "Usage: %s initialPC regFileName instMemFileName dataMemFileName numCycles", argv[0]);
    fprintf(stderr, " enableDataForwarding enableHazardDetection\n");
    fflush(stdout);
    exit(-1);
  }

  const std::int32_t initialPC = (std::int32_t)(atoll(argv[1])) - 4;
  const char *regFileName = argv[2];
  const char *instMemFileName = argv[3];
  const char *dataMemFileName = argv[4];
  const std::uint64_t numCycles = (std::uint64_t)atoll(argv[5]);
  const bool enableDataForwarding = (atol(argv[6]) != 0);
  const bool enableHazardDetection = (atol(argv[7]) != 0);

  PipelinedCPU *cpu = new PipelinedCPU(initialPC, regFileName, instMemFileName, dataMemFileName,
                                       enableDataForwarding, enableHazardDetection);

  cpu->printPVS();
  for (size_t i = 0; i < numCycles; i++) {
    cpu->advanceCycle();
    cpu->printPVS();
  }

  delete cpu;

  return 0;
}


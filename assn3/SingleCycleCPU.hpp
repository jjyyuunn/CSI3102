#ifndef __SINGLE_CYCLE_CPU_HPP__
#define __SINGLE_CYCLE_CPU_HPP__

#include "CPU.hpp"

class SingleCycleCPU : public CPU {
  public:
    SingleCycleCPU(
      const std::uint32_t initialPC, const char *regFileName,
      const char *instMemFileName, const char *dataMemFileName
    ) : CPU(initialPC, regFileName, instMemFileName, dataMemFileName) { }
    virtual void advanceCycle();
};

#endif


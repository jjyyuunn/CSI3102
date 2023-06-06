SRCS = RegisterFile.cpp Memory.cpp

.PHONY: all
all: testSingleCycleCPU testPipelinedCPU

testSingleCycleCPU: testSingleCycleCPU.cpp SingleCycleCPU.cpp $(SRCS)
	g++ -m32 -std=c++11 -o $@ $^

testPipelinedCPU: testPipelinedCPU.cpp PipelinedCPU.cpp $(SRCS)
	g++ -m32 -std=c++11 -o $@ $^

.PHONY: clean
clean:
	rm -f testSingleCycleCPU testPipelinedCPU


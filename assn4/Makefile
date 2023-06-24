SRCS = RegisterFile.cpp Memory.cpp

.PHONY: all
all: testPipelinedCPU

testPipelinedCPU: testPipelinedCPU.cpp PipelinedCPU.cpp $(SRCS)
	g++ -m32 -std=c++11 -o $@ $^

.PHONY: clean
clean:
	rm -f testPipelinedCPU


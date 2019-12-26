NVCC	:=nvcc --cudart=static -ccbin g++
CFLAGS	:=-O3 -std=c++11
ARCHES	:=-gencode arch=compute_70,code=\"compute_70,sm_70\"
INC_DIR	:=-I/usr/local/cuda/samples/common/inc
LIB_DIR	:=
LIBS	:=-lcublas -lnvidia-ml

SOURCES := nvml_cublas \

all: $(SOURCES)
.PHONY: all

nvml_cublas: nvml_cublas.cu
	$(NVCC) $(CFLAGS) $(INC_DIR) $(LIB_DIR) ${ARCHES} $^ -o $@ $(LIBS)
	
clean:
	rm -f $(SOURCES)

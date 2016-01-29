LLVM_ROOT ?= /usr

CMAKE_VARS = \
	-DCMAKE_PREFIX_PATH=$(LLVM_ROOT) \
	-DCMAKE_MODULE_PATH=$(LLVM_ROOT)/share/llvm/cmake \

LLVM_BLD_DIR = LLVM/bld

all: llvm

llvm:
	mkdir -p $(LLVM_BLD_DIR)
	cd $(LLVM_BLD_DIR) && cmake $(CMAKE_VARS) .. && make

llvm.clean:
	rm -rf $(LLVM_BLD_DIR)

clean: llvm.clean

.PHONY: llvm llvm.clean

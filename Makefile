LLVM_DIR = LLVM

ifneq ($(DINO_DEBUG_LOG),)
  CMAKE_VARS += -DDEBUG_LOG=$(DINO_DEBUG_LOG)
endif

include $(MAKER_ROOT)/Makefile.llvm

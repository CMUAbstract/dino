include $(MEMENTOS_ROOT)/Makefile.flags

override SRC_ROOT = ../../src

LIB = libdino

OBJECTS = \
	dino.o \

override CFLAGS += \
	-I$(SRC_ROOT)/include \

override DEPS += libmementos
DEP_ROOT_libmementos ?= $(MEMENTOS_ROOT)

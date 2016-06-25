cmake_minimum_required(VERSION 2.8)

option(AUTORUN_PASSES "Register passes so that OPT runs them automatically" 0)

if(AUTORUN_PASSES)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DAUTORUN_PASSES")
  message(STATUS "Registering passes for autorun")
endif()

add_llvm_loadable_module(DINO
	DINO.cpp
	DINOTaskSplit.cpp
	DINOTaskCost.cpp
	DINOTaskBoundaries.cpp
	DINOVersioner.cpp
	DINOComplexityAnalysis.cpp
)

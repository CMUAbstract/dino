cmake_minimum_required(VERSION 2.8)

option(AUTORUN_PASSES "Register passes so that OPT runs them automatically" 0)

if(AUTORUN_PASSES)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DAUTORUN_PASSES")
  message(STATUS "Registering passes for autorun")
endif()

option(DEBUG_LOG "Enable debug output from passes" 0)

if(DEBUG_LOG)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DDINO_VERBOSE")
  message(STATUS "Enabled debug output from passes")
endif()

add_llvm_loadable_module(DINO
	DINO.cpp
	DINOTaskSplit.cpp
	DINOTaskCost.cpp
	DINOTaskBoundaries.cpp
	DINOVersioner.cpp
	DINOComplexityAnalysis.cpp
)

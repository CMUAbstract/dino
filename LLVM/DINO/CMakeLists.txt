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

add_library(DINO MODULE
	DINO.cpp
	DINOTaskSplit.cpp
	DINOTaskCost.cpp
	DINOTaskBoundaries.cpp
	DINOVersioner.cpp
	DINOComplexityAnalysis.cpp
)

# Use C++11 to compile our pass (i.e., supply -std=c++11).
target_compile_features(DINO PRIVATE cxx_range_for cxx_auto_type)

# LLVM is (typically) built with no C++ RTTI. We need to match that;
# otherwise, we'll get linker errors about missing RTTI data.
set_target_properties(DINO PROPERTIES
    COMPILE_FLAGS "-fno-rtti"
)

# Get proper shared-library behavior (where symbols are not necessarily
# resolved when the shared library is linked) on OS X.
if(APPLE)
	set_target_properties(DINO PROPERTIES
        LINK_FLAGS "-undefined dynamic_lookup"
    )
endif(APPLE)

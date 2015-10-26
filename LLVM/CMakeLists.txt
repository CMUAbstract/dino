# Call like cmake -DDINO_LLVM_DIR=/path/to/llvm
#        or cmake -G Xcode -DDINO_LLVM_DIR=/path/to/llvm

cmake_minimum_required(VERSION 2.8)

set(LLVM_DIR ${DINO_LLVM_DIR}/share/llvm/cmake)

# hack for Windows
if(WIN32)
  set(CMAKE_LEGACY_CYGWIN_WIN32 0)
  set(CYGWIN 1)
endif(WIN32)

find_package(LLVM)

# AddLLVM needs these
set(LLVM_RUNTIME_OUTPUT_INTDIR ${CMAKE_BINARY_DIR}/${CMAKE_CFG_INTDIR}/bin)
set(LLVM_LIBRARY_OUTPUT_INTDIR ${CMAKE_BINARY_DIR}/${CMAKE_CFG_INTDIR}/lib)

include(HandleLLVMOptions) # important: sets compiler flags

include(AddLLVM)

add_definitions(${LLVM_DEFINITIONS})
include_directories(${LLVM_INCLUDE_DIRS})
include_directories(/usr/include)
link_directories(${LLVM_LIBRARY_DIRS})

add_subdirectory(DINO)

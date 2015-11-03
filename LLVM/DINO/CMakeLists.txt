cmake_minimum_required(VERSION 2.8)

add_llvm_loadable_module(DINO
	DINO.cpp
	DINOTaskSplit.cpp
	DINOTaskCost.cpp
	DINOTaskBoundaries.cpp
	DINOVersioner.cpp
	DINOComplexityAnalysis.cpp
)

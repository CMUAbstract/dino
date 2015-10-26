# DINO Passes for LLVM

Our LLVM passes are built "out of source," meaning you can install LLVM
separately and simply load our passes as modules with LLVM's opt tool.

First build LLVM and install it in /path/to/llvm.

Then, in this directory, run

    $ LLVM_ROOT=/path/to/llvm
    $ cmake -DCMAKE_BUILD_TYPE="Debug" -DDINO_LLVM_DIR=$LLVM_ROOT \
    -DCMAKE_PREFIX_PATH=$LLVM_ROOT -DCMAKE_INSTALL_PREFIX=$LLVM_ROOT \
    -DCMAKE_MODULE_PATH=$LLVM_ROOT/share/llvm/cmake .
    $ make

Then you can load the passes by invoking Clang with

    $ clang -Xclang -load -Xclnag $LLVM_ROOT/lib/DINO.so ...

or via opt (TODO: document this).

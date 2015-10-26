#!/bin/sh
echo | "$GCC" -v -x c -E - 2>&1 | awk '/#include <\.\.\.>/{flag=1;next}/End of search list/{flag=0}flag' | awk '{sub(/^ +/, ""); printf " -I%s", $0}'


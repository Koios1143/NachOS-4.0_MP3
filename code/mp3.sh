#!/bin/sh

# Check if output is a terminal, if yes, add color
if [ -t 1 ]; then
    YELLOW="\033[1;33m"
    BLUE="\033[1;34m"
    RED='\033[0;31m'
    NORMAL="\033[0m"
    GREEN="\033[1;32m"
else
    YELLOW=""
    BLUE=""
    RED=""
    NORMAL=""
    GREEN=""
fi

# DBG="-d zt"
DBG=""

TC_NORMAL=(\
"-ep hw3t1 0 -ep hw3t2 0 $DBG" \
"-ep hw3t1 50 -ep hw3t2 50 $DBG" \
"-ep hw3t1 50 -ep hw3t2 90 $DBG" \
"-ep hw3t1 100 -ep hw3t2 100 $DBG" \
"-ep hw3t1 40 -ep hw3t2 55 $DBG" \
"-ep hw3t1 40 -ep hw3t2 90 $DBG" \
"-ep hw3t1 90 -ep hw3t2 100 $DBG" \
"-ep hw3t1 60 -ep hw3t3 50 $DBG" \
)

TIMEOUT="timeout 0.75s"

# Check if script is in the right directory
if [ ${PWD##*/} != "code" ]; then 
    echo -e "\n@@@@@ Script should be put in ./code @@@@@\n"
    exit 1
fi

# Build nachos
echo -e "===== Make nachos ====="
cd build.linux/
make clean > /dev/null 2>&1
make -j2 > /dev/null 2>&1
if [ $? -ne 0 ]; then
    echo -e "Nachos built failed"
    exit 1
else
    echo -e "Nachos built successfully"
fi
cd ..

# Build test cases
echo -e "===== Make test cases ====="
cd test
make clean > /dev/null 2>&1
make hw3t1 hw3t2 hw3t3 > /dev/null 2>&1
if [ $? -ne 0 ]; then
    echo -e "Tests built failed"
    exit 1
else
    echo -e "Tests built successfully"
fi

# Start testing
echo -e "===== Start testcases ====="
for ((i=0; i<${#TC_NORMAL[@]}; i++)); do
    echo -e "===== Test case $(($i+1)): ${TC_NORMAL[$i]} ====="
    $TIMEOUT ../build.linux/nachos ${TC_NORMAL[$i]}
done

exit 0


#!/bin/bash

print_header() {
    echo ""
    echo "=========================================================="
    echo " $1"
    echo "=========================================================="
}

print_header "Building project with 'make'..."

make
# Check the exit status of make
if [ $? -ne 0 ]; then
    echo "Make failed! Please fix compilation errors before testing."
    exit 1
fi
echo "Build successful."


print_header "Running Test 1: ./user/cp4_tests/test1 (User Output Only)"
./yalnix ./user/cp4_tests/test1 2>&1 | grep "^User Prog" # Only print lines that start with User Prog



print_header "Running Test 2: ./user/cp4_tests/test2 (User Output Only)"

./yalnix ./user/cp4_tests/test2 2>&1 | grep "^User Prog"


print_header "Test script finished."
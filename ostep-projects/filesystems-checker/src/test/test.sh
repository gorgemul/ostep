#!/bin/bash

prog="./fsck"

function run_test() {
  test_img=$1
  exp_err_msg=$2
  got_err_msg=$("$prog" "$test_img" 2>&1)
  rc=$?

  if [[ "$rc" -ne 1 ]]; then
    echo "Error: image $test_img"
    echo "Error: got rc $rc, expected 1"
    exit 1
  fi

  if [[ "$got_err_msg" != "$exp_err_msg" ]]; then
    echo "Error: image $test_img"
    echo "Error: got message $got_err_msg, expected message $exp_err_msg"
    exit 1
  fi
}

make -C ../../xv6-src fs.img > /dev/null 2>&1 # ignore the output
cp ../../xv6-src/fs.img .
gcc -Wall -Wextra -pedantic -o fsck ../main.c 
gcc -Wall -Wextra -pedantic -o test_builder1 test_builder1.c 
./test_builder1
run_test test1_1.img "ERROR: bad inode."
run_test test1_2.img "ERROR: bad inode."
run_test test1_3.img "ERROR: bad inode."
echo "Success: test1"
rm test1_*.img test_builder1
rm fsck fs.img
make -C ../../xv6-src clean > /dev/null 2>&1

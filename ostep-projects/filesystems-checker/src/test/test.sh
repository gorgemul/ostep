#!/bin/bash

prog="./fsck"

function run_test() {
  check_img=$1
  exp_err_msg=$2
  got_err_msg=$("$prog" "$check_img" 2>&1)
  rc=$?

  if [[ "$rc" -ne 1 ]]; then
    echo "Error: image $check_img"
    echo "Error: got rc $rc, expected 1"
    exit 1
  fi

  if [[ "$got_err_msg" != "$exp_err_msg" ]]; then
    echo "Error: image $check_img"
    echo "Error: got message $got_err_msg, expected message $exp_err_msg"
    exit 1
  fi
}

echo "building xv6 file system image from scratch..."
make -C ../../xv6-src fs.img > /dev/null 2>&1 # ignore the output
echo "xv6 file system build sucess"
cp ../../xv6-src/fs.img .
gcc -Wall -Wextra -pedantic -o fsck ../main.c 

echo "start running test..."
gcc -Wall -Wextra -pedantic -o test_builder1 test_builder1.c test_util.c
./test_builder1
run_test test1_1.img "ERROR: bad inode."
run_test test1_2.img "ERROR: bad inode."
run_test test1_3.img "ERROR: bad inode."
rm test_builder1 test1_*.img
echo "Success: test1"

gcc -Wall -Wextra -pedantic -o test_builder2 test_builder2.c test_util.c
./test_builder2
run_test test2_1.img "ERROR: bad direct address in inode."
run_test test2_2.img "ERROR: bad direct address in inode."
run_test test2_3.img "ERROR: bad direct address in inode."
run_test test2_4.img "ERROR: bad indirect address in inode."
run_test test2_5.img "ERROR: bad indirect address in inode."
rm test_builder2 test2_*.img
echo "Success: test2"

gcc -Wall -Wextra -pedantic -o test_builder3 test_builder3.c test_util.c
./test_builder3
run_test test3_1.img "ERROR: root directory does not exist."
run_test test3_2.img "ERROR: root directory does not exist."
run_test test3_3.img "ERROR: root directory does not exist."
run_test test3_4.img "ERROR: root directory does not exist."
run_test test3_5.img "ERROR: root directory does not exist."
run_test test3_6.img "ERROR: root directory does not exist."
# run_test test3_7.img "ERROR: root directory does not exist."
rm test_builder3 test3_*.img
echo "Success: test3"

rm fsck fs.img
make -C ../../xv6-src clean > /dev/null 2>&1
echo "All test pass!"

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
gcc -Wall -Wextra -pedantic -o fsck ../main.c ../hash_set.c ../hash_map.c

echo "start running test..."
gcc -Wall -Wextra -pedantic -o test_builder1 test_builder1.c test_util.c
./test_builder1
run_test test1_1.img "ERROR: bad inode."
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
run_test test3_7.img "ERROR: root directory does not exist."
rm test_builder3 test3_*.img
echo "Success: test3"

gcc -Wall -Wextra -pedantic -o test_builder4 test_builder4.c test_util.c
./test_builder4
run_test test4_1.img "ERROR: directory not properly formatted."
run_test test4_2.img "ERROR: directory not properly formatted."
run_test test4_3.img "ERROR: directory not properly formatted."
rm test_builder4 test4_*.img
echo "Success: test4"

gcc -Wall -Wextra -pedantic -o test_builder5 test_builder5.c test_util.c
./test_builder5
run_test test5_1.img "ERROR: address used by inode but marked free in bitmap."
run_test test5_2.img "ERROR: address used by inode but marked free in bitmap."
run_test test5_3.img "ERROR: address used by inode but marked free in bitmap."
rm test_builder5 test5_*.img
echo "Success: test5"

gcc -Wall -Wextra -pedantic -o test_builder6 test_builder6.c test_util.c
./test_builder6
run_test test6_1.img "ERROR: bitmap marks block in use but it is not in use."
rm test_builder6 test6_*.img
echo "Success: test6"

gcc -Wall -Wextra -pedantic -o test_builder7 test_builder7.c test_util.c
./test_builder7
run_test test7_1.img "ERROR: direct address used more than once."
run_test test7_2.img "ERROR: direct address used more than once."
rm test_builder7 test7_*.img
echo "Success: test7"

gcc -Wall -Wextra -pedantic -o test_builder8 test_builder8.c test_util.c
./test_builder8
run_test test8_1.img "ERROR: indirect address used more than once."
run_test test8_2.img "ERROR: indirect address used more than once."
rm test_builder8 test8_*.img
echo "Success: test8"

gcc -Wall -Wextra -pedantic -o test_builder9 test_builder9.c test_util.c
./test_builder9
run_test test9_1.img "ERROR: inode marked use but not found in a directory."
run_test test9_2.img "ERROR: inode marked use but not found in a directory."
run_test test9_2.img "ERROR: inode marked use but not found in a directory."
rm test_builder9 test9_*.img
echo "Success: test9"

gcc -Wall -Wextra -pedantic -o test_builder10 test_builder10.c test_util.c
./test_builder10
run_test test10_1.img "ERROR: inode referred to in directory but marked free."
rm test_builder10 test10_*.img
echo "Success: test10"

gcc -Wall -Wextra -pedantic -o test_builder11 test_builder11.c test_util.c
./test_builder11
run_test test11_1.img "ERROR: bad reference count for file."
rm test_builder11 test11_*.img
echo "Success: test11"

gcc -Wall -Wextra -pedantic -o test_builder12 test_builder12.c test_util.c
./test_builder12
run_test test12_1.img "ERROR: directory appears more than once in file system."
rm test_builder12 test12_*.img
echo "Success: test12"

rm fsck fs.img
make -C ../../xv6-src clean > /dev/null 2>&1
echo "All test pass!"

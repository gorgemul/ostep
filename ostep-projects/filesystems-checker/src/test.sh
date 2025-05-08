#!/bin/bash

set -xe

prog="./out"
err_msg=$("$prog" foo 2>&1)

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

make -C ../xv6-src
cp ../xv6-src/fs.img .


run_test foo.img foobar

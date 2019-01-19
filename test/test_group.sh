#!/bin/sh

set -e 

d=files

fail() {
  echo $* >&2
  return 1
}

test() {
  rm -rf $d/tmp
  while [ -n "$1" ]; do
    echo " -- $1 "
    echo $1 >> $d/tmp
    shift
  done
  ../graft_group $d/grorig $d/tmp  >$d/tmpout >$d/tmperr
}    

testok() {
  if ! test $@ ; then 
    fail "failed, should have passed: $1"
  fi
}

testno() {
  if test $@; then
    fail "passed, should have failed: $1"
  fi
}

check() {
  local line=$(grep $1 $d/tmpout)
  if [ "$line" != "$2" ]; then
    fail "got: '$line' expected: '$2'"
  fi
}  

cat >$d/grorig <<EOF
audio:x:101:root
vt:x:102:
storage:x:103:
EOF

testok "audio:x:101:mg"
check audio "audio:x:101:mg,root"

testok "mg:x:1000:mg"
check mg "mg:x:1000:mg"

testok "audio:x:101:mg" "mg:x:1000:mg"
check audio "audio:x:101:mg,root"
check mg "mg:x:1000:mg"


testno "audio:y:101:mg"
testno "root:x:0:"
testno "audio:x:105:"
testno "audio:x:101:foo"

echo " tests OK"

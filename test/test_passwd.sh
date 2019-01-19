#!/bin/sh

set -e

d=files

mkdir -p $d

fail() {
  echo $* >&2
  return 1
}

dotest() {
  rm -rf $d/tmp
  while [ -n "$1" ]; do
    echo $1 >> $d/tmp
    shift
  done
  ../graft_passwd $d/pworig $d/tmp  >$d/tmpout 2>$d/tmperr
}    

testok() {
  if ! dotest $@ ; then 
    fail "failed, should have passed: $1"
  fi
}

testno() {
  if dotest $@; then
    fail "passed, should have failed: $1"
  fi
}


echo "mg:x:1000:1000::/home/mg:bin/bash" > $d/pworig

testok "na:x:1001:1001::/home/na:/bin/bash"
testno "mg:x:1010:1010::/home/mg:/bin/bash"
testno "li:x:1000:1002::/home/li:/bin/bash"
testno "fe:x:0:1000::/home/fe:/bin/bash"
testno "ca:x:1004:0::/home/ca:/bin/bash"
testno "co:y:1005:1005::/home/co:/bin/bash"
testno "ni:x:1006:1006::/bin/ni:/bin/bash"
echo " tests OK"


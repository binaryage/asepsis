#!/bin/bash

TMP=`sw_vers -productVersion|grep '10\.\(8\|9\|10\)'`
if [ $? -eq 0 ]; then
  exit 0
fi
exit 1
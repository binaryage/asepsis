#!/bin/bash

# codesign is not needed under Lion and Mountain Lion
TMP=`sw_vers -productVersion|grep '10\.\(7\|8\)'`
if [ $? -eq 0 ]; then
  exit 0
fi

# codesign can be installed as part of xcode command-line tools
# see http://stackoverflow.com/a/15371967/84283
TMP=`pkgutil --pkg-info=com.apple.pkg.CLTools_Executables`
status=$?
if [ $status -ne 0 ]; then
  xcode-select --install
  exit 1
fi
exit 0

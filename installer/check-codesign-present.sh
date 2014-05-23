#!/bin/bash

# codesign can be installed as part of xcode command-line tools
# see http://stackoverflow.com/a/15371967/84283
TMP=`pkgutil --pkg-info=com.apple.pkg.CLTools_Executables`
status=$?
if [ $status -ne 0 ]; then
  xcode-select --install
  exit 1
fi
exit 0

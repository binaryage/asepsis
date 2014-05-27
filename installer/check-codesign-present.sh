#!/bin/bash

# this is just a hidden tweak for bypassing this test if needed
if [ -f ~/.skip-asepsis-codesign-check ]; then
  exit 0
fi

# codesign is not needed under Lion and Mountain Lion
TMP=`sw_vers -productVersion|grep '10\.\(7\|8\)'`
if [ $? -eq 0 ]; then
  exit 0
fi

# under 10.9 /usr/bin/codesign is a stub, it raises Xcode command-line tools installation dialog if used first time
#
# codesign can be installed by installing Xcode or as part of Xcode command-line tools (without full Xcode)
# see http://stackoverflow.com/a/15371967/84283
#
# we want to detect codesign presence here and stop installer if codesign is not available
# in case of failure we want to open installation dialog for Xcode command-line tools
#
xcrun --find codesign # in case of failure raises installation dialog and returns non-zero exit code (which will be the exit code of this script)

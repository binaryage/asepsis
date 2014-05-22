#!/bin/bash

if [ -z `which codesign` ]; then
  xcode-select --install
  exit 1
fi
exit 0
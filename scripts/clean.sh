#!/bin/sh

if [ ${#PROJECT_DIR} -le 10 ]; then echo "error: set \$PROJECT_DIR env variable" ; exit 1; fi

BIN="$PROJECT_DIR/bin"

rm -rf "$BIN"
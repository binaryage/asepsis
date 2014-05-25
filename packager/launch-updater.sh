#!/bin/sh

# this script is normally launched during startup
# the system may be busy doing lots of operations, network might be not available yet
# give system some time before launching the update check
sleep 180 # 3 minutes

# run the updater check
/Library/Application\ Support/Asepsis/AsepsisUpdater.app/Contents/MacOS/AsepsisUpdater
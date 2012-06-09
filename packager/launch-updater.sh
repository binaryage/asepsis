#!/bin/sh

# this should be launched during system boot, machine is busy doing lots of tasks => don't hurry
sleep 10

# wait for network
/usr/sbin/ipconfig waitall

# run the updater check
/Library/Application\ Support/Asepsis/AsepsisUpdater.app/Contents/MacOS/AsepsisUpdater
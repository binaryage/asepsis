#!/bin/sh

if [ ${#PROJECT_DIR} -le 10 ]; then echo "error: set \$PROJECT_DIR env variable" ; exit 1; fi
  
BIN="$PROJECT_DIR/bin"
mkdir -p "$BIN"
BIN=`cd "$BIN";pwd` # normalize path

TARGET="$BIN/Asepsis"
DAEMON="$TARGET/asepsisd"
TEST="$TARGET/asepsisTest"
WRAPPER="$TARGET/DesktopServicesPrivWrapper"
UPDATER="$TARGET/AsepsisUpdater.app"
UNINSTALLER="$TARGET/Asepsis Uninstaller.app"
PACKAGER="$PROJECT_DIR/packager"

echo "build products dir is $BUILT_PRODUCTS_DIR"
echo "assembling final products into $BIN"

mkdir -p "$TARGET"

cp "$PACKAGER"/com.binaryage.*.plist "$TARGET"
cp "$PACKAGER/finish-uninstall.sh" "$TARGET"
cp "$PACKAGER/finish-kext-uninstall.sh" "$TARGET"
cp "$PACKAGER/launch-updater.sh" "$TARGET"
cp -Rf "$BUILT_PRODUCTS_DIR/asepsisd" "$DAEMON"
cp -Rf "$BUILT_PRODUCTS_DIR/asepsisTest" "$TEST"
cp -Rf "$BUILT_PRODUCTS_DIR/DesktopServicesPrivWrapper.framework/Versions/A/DesktopServicesPrivWrapper" "$WRAPPER"
cp -Rf "$BUILT_PRODUCTS_DIR/AsepsisUpdater.app" "$UPDATER"
cp "$PACKAGER/install_name_tool" "$TARGET"

cp -Rf "$BUILT_PRODUCTS_DIR/TotalFinder Uninstaller.app/" "$UNINSTALLER"
mv "$UNINSTALLER/Contents/MacOS/TotalFinder Uninstaller" "$UNINSTALLER/Contents/MacOS/Asepsis Uninstaller"

install_name_tool -change "/System/Library/PrivateFrameworks/DesktopServicesPriv.framework/Versions/A/DesktopServicesPriv" "/System/Library/PrivateFrameworks/DesktopServicesPriv.framework/Versions/A_/DesktopServicesPriv" "$WRAPPER"

rm "$UPDATER/Contents/Frameworks/Sparkle.framework/Headers"
rm -rf "$UPDATER/Contents/Frameworks/Sparkle.framework/Versions/A/Headers"

cp -Rf "$PROJECT_DIR/ctl" "$TARGET"
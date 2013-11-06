#!/bin/sh

BIN="$PROJECT_DIR/../bin"
mkdir -p "$BIN"
BIN=`cd "$BIN";pwd` # normalize path

TARGET="$BIN/Asepsis"
DAEMON="$TARGET/asepsisd"
TEST="$TARGET/asepsisTest"
WRAPPER="$TARGET/DesktopServicesPrivWrapper"
UPDATER="$TARGET/AsepsisUpdater.app"

echo "build products dir is $BUILT_PRODUCTS_DIR"
echo "assembling final products in $BIN"

mkdir -p "$TARGET"

cp "$PROJECT_DIR"/com.binaryage.*.plist "$TARGET"
cp "$PROJECT_DIR/finish-uninstall.sh" "$TARGET"
cp "$PROJECT_DIR/finish-kext-uninstall.sh" "$TARGET"
cp "$PROJECT_DIR/launch-updater.sh" "$TARGET"
cp -Rf "$BUILT_PRODUCTS_DIR/asepsisd" "$DAEMON"
cp -Rf "$BUILT_PRODUCTS_DIR/asepsisTest" "$TEST"
cp -Rf "$BUILT_PRODUCTS_DIR/DesktopServicesPrivWrapper.framework/Versions/A/DesktopServicesPrivWrapper" "$WRAPPER"
cp -Rf "$BUILT_PRODUCTS_DIR/AsepsisUpdater.app" "$UPDATER"
cp "$PROJECT_DIR/install_name_tool" "$TARGET"
cp "$PROJECT_DIR/codesign" "$TARGET"

rm "$UPDATER/Contents/Frameworks/Sparkle.framework/Headers"
rm -rf "$UPDATER/Contents/Frameworks/Sparkle.framework/Versions/A/Headers"

cp -Rf "$PROJECT_DIR/../ctl" "$TARGET"
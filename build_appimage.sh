#!/bin/bash
# build_appimage.sh - Package PE-bear as a self-contained Linux AppImage.
#
# Usage:
#   ./build_appimage.sh                 # Qt6 (recommended)
#   QT_VARIANT=qt5 ./build_appimage.sh  # Qt5 build
#   VERSION=0.7.2 ./build_appimage.sh   # override version in output name
#
# Build it on the OLDEST glibc you want to support (e.g. Ubuntu 20.04):
# AppImages are forward-compatible with newer glibc, never backward.
set -euo pipefail

# ---- Config --------------------------------------------------------------
VERSION="${VERSION:-0.7.2}"
QT_VARIANT="${QT_VARIANT:-qt6}"        # qt6 (default) | qt5
ARCH="$(uname -m)"                     # x86_64
JOBS="$(nproc)"
BUILD_DIR="build_appimage"
APPDIR="$PWD/AppDir"
TOOLS_DIR="$PWD/tools"

LINUXDEPLOY="$TOOLS_DIR/linuxdeploy-${ARCH}.AppImage"
LINUXDEPLOY_QT="$TOOLS_DIR/linuxdeploy-plugin-qt-${ARCH}.AppImage"
LD_BASE="https://github.com/linuxdeploy"
LINUXDEPLOY_URL="$LD_BASE/linuxdeploy/releases/download/continuous/linuxdeploy-${ARCH}.AppImage"
LINUXDEPLOY_QT_URL="$LD_BASE/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-${ARCH}.AppImage"

# Containers/CI frequently lack FUSE; let the tool AppImages self-extract.
export APPIMAGE_EXTRACT_AND_RUN=1

fetch() {  # fetch <dest> <url>
    if command -v wget >/dev/null 2>&1; then wget -q -O "$1" "$2"
    else curl -fsSL -o "$1" "$2"; fi
}

# ---- 1. Configure & build ------------------------------------------------
CMAKE_QT_FLAGS=()
[ "$QT_VARIANT" = "qt5" ] && CMAKE_QT_FLAGS+=(-DUSE_QT5=ON)

echo "==> Configuring ($QT_VARIANT)"
cmake -S . -B "$BUILD_DIR" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr \
    "${CMAKE_QT_FLAGS[@]}"

echo "==> Building with $JOBS jobs"
cmake --build "$BUILD_DIR" --parallel "$JOBS"

# ---- 2. Install into a clean AppDir --------------------------------------
echo "==> Installing into AppDir"
rm -rf "$APPDIR"
DESTDIR="$APPDIR" cmake --install "$BUILD_DIR"

# ---- 3. Fetch linuxdeploy + Qt plugin (cached in ./tools) ----------------
mkdir -p "$TOOLS_DIR"
[ -x "$LINUXDEPLOY" ]    || { echo "==> Downloading linuxdeploy";        fetch "$LINUXDEPLOY"    "$LINUXDEPLOY_URL";    chmod +x "$LINUXDEPLOY"; }
[ -x "$LINUXDEPLOY_QT" ] || { echo "==> Downloading linuxdeploy-plugin-qt"; fetch "$LINUXDEPLOY_QT" "$LINUXDEPLOY_QT_URL"; chmod +x "$LINUXDEPLOY_QT"; }

# linuxdeploy discovers plugins named linuxdeploy-plugin-<name>* on PATH.
export PATH="$TOOLS_DIR:$PATH"

# Point the Qt plugin at the right qmake when several Qt versions coexist.
if [ "$QT_VARIANT" = "qt6" ]; then
    export QMAKE="${QMAKE:-$(command -v qmake6 || true)}"
else
    export QMAKE="${QMAKE:-$(command -v qmake || true)}"
fi

# ---- 4. Normalize the icon to a valid hicolor size -----------------------
# linuxdeploy rejects icons that aren't a standard resolution (8..512). The
# source pixmap is oversized, so emit a 256x256 copy in the spec-correct path.
ICON_SRC="$APPDIR/usr/share/pixmaps/net.hasherezade.pe-bear.png"
ICON_DEPLOY="$ICON_SRC"
if command -v magick >/dev/null 2>&1 || command -v convert >/dev/null 2>&1; then
    IM="$(command -v magick || command -v convert)"
    ICON_256="$APPDIR/usr/share/icons/hicolor/256x256/apps/net.hasherezade.pe-bear.png"
    mkdir -p "$(dirname "$ICON_256")"
    "$IM" "$ICON_SRC" -resize 256x256 "$ICON_256"
    ICON_DEPLOY="$ICON_256"
else
    echo "WARNING: ImageMagick not found; passing the source icon as-is."
    echo "         Install imagemagick, or resize the icon to 256x256 manually."
fi

# ---- 5. Bundle & produce the AppImage ------------------------------------
# Deploy the Wayland platform plugins alongside the default xcb one, so the
# AppImage runs natively on pure-Wayland sessions (no XWayland needed). These
# require the Qt Wayland packages (qt6-wayland / qtwayland5) in the build env.
export EXTRA_PLATFORM_PLUGINS="libqwayland-egl.so;libqwayland-generic.so"
export EXTRA_QT_MODULES="waylandcompositor"

echo "==> Bundling dependencies & producing AppImage"
export OUTPUT="PE-bear_${VERSION}_${QT_VARIANT}_${ARCH}_linux.AppImage"

"$LINUXDEPLOY" \
    --appdir "$APPDIR" \
    --plugin qt \
    --executable "$APPDIR/usr/bin/PE-bear" \
    --desktop-file "$APPDIR/usr/share/applications/net.hasherezade.pe-bear.desktop" \
    --icon-file "$ICON_DEPLOY" \
    --output appimage

echo "==> Done: $OUTPUT"

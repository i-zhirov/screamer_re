#!/bin/sh
# Install the LX/LE loader extension into the Ghidra user directory.
#
# The Nix store is read-only, so the extension goes into the Ghidra user
# settings directory.  Run this inside `nix develop .#ghidra`:
#
#     nix develop .#ghidra --command sh tools/ghidra/install_loader.sh
#
# The loader is the one released for Ghidra 12.0.x:
# https://github.com/yetmorecode/ghidra-lx-loader
#
# Without the loader, Ghidra reads a DOS/4GW executable as a 16-bit MZ image and
# misses the 32-bit code.

set -e

LOADER_VERSION="v12.0.1"
LOADER_URL="https://github.com/yetmorecode/ghidra-lx-loader/releases/download/${LOADER_VERSION}/ghidra_12.0.1_PUBLIC_20260129_ghidra-lx-loader.zip"

if [ -z "$GHIDRA_INSTALL_DIR" ]; then
    GHIDRA_INSTALL_DIR="$(dirname "$(dirname "$(command -v ghidra)")")"
fi
GHIDRA_VERSION="$(sed -n 's/^application.version=//p' "$GHIDRA_INSTALL_DIR/Ghidra/application.properties")"
if [ -z "$GHIDRA_VERSION" ]; then
    echo "cannot read the Ghidra version from $GHIDRA_INSTALL_DIR" >&2
    exit 1
fi

# The Nix package appends _NIX to the application name, so the user directory
# carries that suffix.
if [ -n "$GHIDRA_USER_DIR" ]; then
    USER_DIR="$GHIDRA_USER_DIR"
elif [ "$(uname)" = "Darwin" ]; then
    USER_DIR="$HOME/Library/ghidra/ghidra_${GHIDRA_VERSION}_NIX"
else
    USER_DIR="${XDG_CONFIG_HOME:-$HOME/.config}/ghidra/ghidra_${GHIDRA_VERSION}_NIX"
fi

CACHE="work/ghidra/lx-loader.zip"
if [ ! -f "$CACHE" ]; then
    mkdir -p "$(dirname "$CACHE")"
    echo "downloading the loader for Ghidra 12.0.x"
    curl -sL -o "$CACHE" "$LOADER_URL"
fi

mkdir -p "$USER_DIR/Extensions"
unzip -oq "$CACHE" -d "$USER_DIR/Extensions"

# The release declares the version it was built for.  Set the version of the
# Ghidra in use, or Ghidra refuses to load the extension.
PROPERTIES="$USER_DIR/Extensions/ghidra-lx-loader/extension.properties"
sed -i.bak "s/^version=.*/version=${GHIDRA_VERSION}/" "$PROPERTIES"
rm -f "$PROPERTIES.bak"

echo "installed the LX/LE loader for Ghidra $GHIDRA_VERSION"
echo "user directory: $USER_DIR"

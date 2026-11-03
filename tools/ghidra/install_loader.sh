#!/bin/sh
# Install or check the LX/LE loader extension in the Ghidra user directory.
#
# The Nix store is read-only, so the extension goes into the Ghidra user
# settings directory.  Run this inside `nix develop .#cross-check`:
#
#     nix develop .#cross-check --command sh tools/ghidra/install_loader.sh
#
# Use `--check` to verify the installed extension and its version without a
# download.
#
# The loader is the one released for Ghidra 12.0.x:
# https://github.com/yetmorecode/ghidra-lx-loader
#
# Without the loader, Ghidra reads a DOS/4GW executable as a 16-bit MZ image and
# misses the 32-bit code.

set -e

LOADER_VERSION="v12.0.1"
LOADER_URL="https://github.com/yetmorecode/ghidra-lx-loader/releases/download/${LOADER_VERSION}/ghidra_12.0.1_PUBLIC_20260129_ghidra-lx-loader.zip"
INSTALL_COMMAND="nix develop .#cross-check --command sh tools/ghidra/install_loader.sh"

CHECK=0
if [ "$1" = "--check" ]; then
    CHECK=1
fi

if [ -z "$GHIDRA_INSTALL_DIR" ]; then
    GHIDRA_INSTALL_DIR="$(dirname "$(dirname "$(command -v ghidra)")")"
fi

PROPERTIES_FILE="$GHIDRA_INSTALL_DIR/Ghidra/application.properties"
if [ ! -r "$PROPERTIES_FILE" ]; then
    if [ "$CHECK" = "1" ]; then
        echo "cannot read the Ghidra version from $PROPERTIES_FILE" >&2
        echo "install the loader with: $INSTALL_COMMAND" >&2
    else
        echo "cannot read the Ghidra version from $GHIDRA_INSTALL_DIR" >&2
    fi
    exit 2
fi

GHIDRA_VERSION="$(sed -n 's/^application.version=//p' "$PROPERTIES_FILE")"
if [ -z "$GHIDRA_VERSION" ]; then
    if [ "$CHECK" = "1" ]; then
        echo "cannot read the Ghidra version from $PROPERTIES_FILE" >&2
        echo "install the loader with: $INSTALL_COMMAND" >&2
    else
        echo "cannot read the Ghidra version from $GHIDRA_INSTALL_DIR" >&2
    fi
    exit 2
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

PROPERTIES="$USER_DIR/Extensions/ghidra-lx-loader/extension.properties"

if [ "$CHECK" = "1" ]; then
    if [ ! -r "$PROPERTIES" ]; then
        echo "the LX/LE loader is not installed in $USER_DIR" >&2
        echo "install the loader with: $INSTALL_COMMAND" >&2
        exit 2
    fi
    EXTENSION_VERSION="$(sed -n 's/^version=//p' "$PROPERTIES")"
    if [ "$EXTENSION_VERSION" != "$GHIDRA_VERSION" ]; then
        echo "the LX/LE loader version $EXTENSION_VERSION does not match Ghidra $GHIDRA_VERSION" >&2
        echo "install the loader with: $INSTALL_COMMAND" >&2
        exit 2
    fi
    echo "ok: $PROPERTIES (version $EXTENSION_VERSION)"
    exit 0
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
sed -i.bak "s/^version=.*/version=${GHIDRA_VERSION}/" "$PROPERTIES"
rm -f "$PROPERTIES.bak"

echo "installed the LX/LE loader for Ghidra $GHIDRA_VERSION"
echo "user directory: $USER_DIR"

#!/bin/sh
# Run the identity script or check the Ghidra setup for the cross-check.
#
# Use `--check` to verify `analyzeHeadless`, the Ghidra install, the identity
# script, and the LX/LE loader extension.  The check does not download.
#
# Use a run to import an executable and print the identity keys:
#
#     sh tools/ghidra/identity.sh --exe STARTH.EXE --project-dir work/proj \
#         --main 0x10020 --call-site 0xd4364
#
# Run both modes inside `nix develop .#cross-check`.

set -e

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
IDENTITY_SCRIPT="$SCRIPT_DIR/LeIdentity.java"
INSTALL_SCRIPT="$SCRIPT_DIR/install_loader.sh"
INSTALL_COMMAND="nix develop .#cross-check --command sh tools/ghidra/install_loader.sh"
CHECK_COMMAND="nix develop .#cross-check --command sh tools/ghidra/identity.sh --check"
MARKER="=== identity"
KEYS="entry page0_module page0_file main caller call_site call_target"
KEY_PATTERN="^(entry|page0_module|page0_file|main|caller|call_site|call_target)="
PROJECT_NAME="Screamer"
OUTPUT_NAME="identity.txt"
FILTERED_NAME="identity-keys.txt"

usage() {
    echo "usage: $0 --check" >&2
    echo "       $0 --exe PATH --project-dir DIR --main ADDR --call-site ADDR" >&2
}

check_mode() {
    if ! command -v analyzeHeadless >/dev/null 2>&1; then
        echo "analyzeHeadless is not on PATH" >&2
        echo "run the checks in: $CHECK_COMMAND" >&2
        exit 2
    fi
    if [ -z "$GHIDRA_INSTALL_DIR" ]; then
        echo "GHIDRA_INSTALL_DIR is not set" >&2
        echo "install the loader with: $INSTALL_COMMAND" >&2
        exit 2
    fi
    PROPERTIES_FILE="$GHIDRA_INSTALL_DIR/Ghidra/application.properties"
    if [ ! -r "$PROPERTIES_FILE" ]; then
        echo "cannot read the Ghidra version from $PROPERTIES_FILE" >&2
        echo "install the loader with: $INSTALL_COMMAND" >&2
        exit 2
    fi
    GHIDRA_VERSION="$(sed -n 's/^application.version=//p' "$PROPERTIES_FILE")"
    if [ -z "$GHIDRA_VERSION" ]; then
        echo "cannot read the Ghidra version from $PROPERTIES_FILE" >&2
        echo "install the loader with: $INSTALL_COMMAND" >&2
        exit 2
    fi
    if [ ! -r "$IDENTITY_SCRIPT" ]; then
        echo "cannot read the identity script at $IDENTITY_SCRIPT" >&2
        echo "install the loader with: $INSTALL_COMMAND" >&2
        exit 2
    fi
    if ! sh "$INSTALL_SCRIPT" --check; then
        exit 2
    fi
    echo "ok: Ghidra $GHIDRA_VERSION"
    exit 0
}

fail_run() {
    echo "$1" >&2
    rm -rf "$PROJECT_DIR"
    exit 2
}

run_mode() {
    EXE=""
    PROJECT_DIR=""
    MAIN=""
    CALL_SITE=""
    while [ "$#" -gt 0 ]; do
        case "$1" in
            --exe)
                [ "$#" -ge 2 ] || { usage; exit 2; }
                EXE="$2"
                shift 2
                ;;
            --project-dir)
                [ "$#" -ge 2 ] || { usage; exit 2; }
                PROJECT_DIR="$2"
                shift 2
                ;;
            --main)
                [ "$#" -ge 2 ] || { usage; exit 2; }
                MAIN="$2"
                shift 2
                ;;
            --call-site)
                [ "$#" -ge 2 ] || { usage; exit 2; }
                CALL_SITE="$2"
                shift 2
                ;;
            *)
                echo "unknown argument: $1" >&2
                usage
                exit 2
                ;;
        esac
    done
    if [ -z "$EXE" ] || [ -z "$PROJECT_DIR" ] || [ -z "$MAIN" ] \
            || [ -z "$CALL_SITE" ]; then
        usage
        exit 2
    fi
    if [ ! -f "$EXE" ]; then
        echo "no executable at $EXE" >&2
        exit 2
    fi
    if ! command -v analyzeHeadless >/dev/null 2>&1; then
        echo "analyzeHeadless is not on PATH" >&2
        echo "run the checks in: $CHECK_COMMAND" >&2
        exit 2
    fi

    mkdir -p "$PROJECT_DIR"
    OUTPUT="$PROJECT_DIR/$OUTPUT_NAME"
    FILTERED="$PROJECT_DIR/$FILTERED_NAME"

    STATUS=0
    analyzeHeadless "$PROJECT_DIR" "$PROJECT_NAME" \
        -import "$EXE" \
        -overwrite \
        -scriptPath "$SCRIPT_DIR" \
        -postScript LeIdentity.java --main "$MAIN" --call-site "$CALL_SITE" \
        > "$OUTPUT" 2>&1 || STATUS=$?

    sed -n '/=== identity/,$p' "$OUTPUT" \
        | sed -e 's/^INFO  LeIdentity\.java> //' \
              -e 's/ (GhidraScript)[[:space:]]*$//' \
              -e 's/[[:space:]]*$//' > "$FILTERED"

    if [ "$STATUS" -ne 0 ]; then
        fail_run "analyzeHeadless failed with status $STATUS"
    fi
    if grep -q '^error:' "$FILTERED"; then
        fail_run "the identity script reported an error"
    fi
    if ! grep -q "^$MARKER$" "$FILTERED"; then
        fail_run "the output has no '$MARKER' marker"
    fi
    for KEY in $KEYS; do
        if ! grep -q "^$KEY=" "$FILTERED"; then
            fail_run "the output has no $KEY key"
        fi
    done

    grep -E "^$MARKER\$|$KEY_PATTERN" "$FILTERED"
    exit 0
}

if [ "$#" -eq 0 ]; then
    usage
    exit 2
fi

case "$1" in
    --check)
        if [ "$#" -ne 1 ]; then
            usage
            exit 2
        fi
        check_mode
        ;;
    --exe | --project-dir | --main | --call-site)
        run_mode "$@"
        ;;
    *)
        usage
        exit 2
        ;;
esac

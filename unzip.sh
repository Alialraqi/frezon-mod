#!/bin/sh
set -eu

ARCHIVE="${1:-project.zip}"
DEST="${2:-SeedLock-Project}"

if [ ! -f "$ARCHIVE" ]; then
  echo "Error: archive not found: $ARCHIVE" >&2
  echo "Usage: $0 [project.zip] [destination-folder]" >&2
  exit 1
fi

mkdir -p "$DEST"
unzip -o "$ARCHIVE" -d "$DEST"
echo "Extracted '$ARCHIVE' into '$DEST'"
